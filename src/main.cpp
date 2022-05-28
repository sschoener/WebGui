#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <GLES3/gl3.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>

//#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <vector>

// node graph
#include "imgui_internal.h"

// utility
#include "allocator.h"

SDL_Window* g_window;
SDL_GLContext g_gl_context;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
ImGuiContext* imgui = 0;
bool show_demo_window = false;
bool show_another_window = false;

EM_JS(int, canvas_get_width, (), {
    return Module.canvas.width;
});

EM_JS(int, canvas_get_height, (), {
    return Module.canvas.height;
});

EM_JS(void, resizeCanvas, (), {
    js_resizeCanvas();
});

EM_JS(bool, SendMessage, (uint8_t* data, int size), {
    if (Module.socket && Module.socket.readyState == 1 /* OPEN */)
    {
        Module.socket.send(Module.HEAPU8.subarray(data, data + size));
        return true;
    }
    return false;
});

EM_JS(int, GetMessages, (void** data, int32_t** sizes, int maxMessages), {
    const msgs = Module.incomingMessages;
    const n = maxMessages > msgs.length ? msgs.length : n;
    let dataOffset = data / 4;
    let sizeOffset = sizes / 4;
    for (let i = 0; i < n; i++) {
        Module.HEAPU32[dataOffset] = msgs[i].ptr;
        Module.HEAPU32[sizeOffset] = msgs[i].len;
        dataOffset++;
        sizeOffset++;
    }
    msgs.splice(0, n);
    return n;
});

PersistentLinearAllocator PersistentAlloc;

enum class MessageType : uint32_t {

};

struct MessageInfo
{
    size_t Index;
    MessageType Type;
};
std::vector<uint8_t> MessageBuffer;
std::vector<MessageInfo> MessageStack;
std::vector<MessageInfo> Messages;
void BeginMessage(MessageType type)
{
    assert(MessageStack.empty());
    MessageStack.emplace_back();
    MessageInfo& info = MessageStack.back();
    info.Type = type;
    size_t size = MessageBuffer.size();
    info.Index = size;
    MessageBuffer.resize(size + 2 * sizeof(uint32_t));
    *(uint32_t*)(MessageBuffer.data() + info.Index) = (uint32_t) type;
}

void EndMessage(MessageType type)
{
    assert(!MessageStack.empty());
    assert(MessageStack.back().Type == type);
    MessageInfo& info = MessageStack.back();
    // compute the size and patch it in
    *(uint32_t*)(MessageBuffer.data() + (info.Index + sizeof(uint32_t))) = MessageBuffer.size() - info.Index - 2 * sizeof(uint32_t);
    MessageStack.pop_back();
    Messages.push_back(info);
}

void DiscardMessage(MessageType type)
{
    assert(!MessageStack.empty());
    assert(MessageStack.back().Type == type);
    MessageInfo& info = MessageStack.back();
    MessageBuffer.resize(info.Index);
    MessageStack.pop_back();
}

void WriteMessage(void* ptr, size_t size)
{
    size_t index = MessageBuffer.size();
    MessageBuffer.resize(index + size);
    uint8_t* dst = (uint8_t*)MessageBuffer.data() + index;
    memcpy(dst, ptr, size);
}

size_t ReserveMessage(size_t size) {
    size_t index = MessageBuffer.size();
    MessageBuffer.resize(index + size);
    return index;
}

template<typename T>
void WriteMessage(const T& t)
{
    WriteMessage((void*)&t, sizeof(T));
}

template<typename T>
size_t ReserveMessage()
{
    return ReserveMessage(sizeof(T));
}

template<typename T>
void WriteReservedMessage(size_t reservation, const T& t)
{
    size_t size = sizeof(T);
    uint8_t* dst = (uint8_t*)MessageBuffer.data() + reservation;
    memcpy(dst, &t, size);
}

void FlushMessageQueue()
{
    assert(MessageStack.empty());
    const size_t totalBufferSize = MessageBuffer.size();
    const size_t numMessages = Messages.size();
    size_t numSent = 0;
    for (size_t s = 0; s < numMessages; s++) {
        const size_t msgStart = Messages[s].Index;
        const size_t msgEnd = s < numMessages - 1 ? Messages[s + 1].Index : totalBufferSize;
        const size_t msgSize = msgEnd - msgStart;
        if (!SendMessage(MessageBuffer.data() + msgStart, (int) msgSize))
            break;
        else
            numSent++;
    }
    if (numSent == numMessages) {
        MessageBuffer.clear();
        Messages.clear();
    } else {
        const size_t sentUntil = Messages[numSent].Index;
        Messages.erase(Messages.begin(), Messages.begin() + numSent);
        MessageBuffer.erase(MessageBuffer.begin(), MessageBuffer.begin() + sentUntil);
    }
}

template<typename T>
struct Array
{
    T* Ptr;
    size_t Length;

    T& operator[](size_t index) {
        assert(index < Length && index >= 0);
        return Ptr[index];
    }

    const T& operator[](size_t index) const {
        assert(index < Length && index >= 0);
        return Ptr[index];
    }
};

struct Span
{
    uint8_t* Data;
    size_t Length;

    uint8_t* Read(size_t n) {
        assert(n <= Length);
        uint8_t* p = Data;
        Data += n;
        Length -= n;
        return p;
    }

    uint32_t Read_uint32()
    {
        return *(uint32_t*)Read(sizeof(uint32_t));
    }

    const char* Read_utf8()
    {
        auto size = Read_uint32();
        char* ptr = (char*)Allocate(PersistentAlloc, size + 1);
        memcpy(ptr, (char*)Read(size), size);
        ptr[size] = 0;
        return ptr;
    }

    const char* ReadDynamic_utf8()
    {
        auto size = Read_uint32();
        char* ptr = (char*)malloc(size + 1);
        memcpy(ptr, (char*)Read(size), size);
        ptr[size] = 0;
        return ptr;
    }
};

uint32_t ProcessMessage(Span& data)
{
    const MessageType type = (MessageType)data.Read_uint32();
    const uint32_t msgSize = data.Read_uint32();
    uint8_t* const origData = data.Data;
    switch (type) {
    }
    return msgSize + 8;
}

static ImRect CutLeft(ImRect& rect, float amount)
{
    ImRect copy = rect;
    rect.Min.x += amount;
    copy.Max.x = rect.Min.x;
    return copy;
}

static ImRect CutRight(ImRect& rect, float amount)
{
    ImRect copy = rect;
    rect.Max.x -= amount;
    copy.Min.x = rect.Max.x;
    return copy;
}

static ImRect CutTop(ImRect& rect, float amount)
{
    ImRect copy = rect;
    rect.Min.y += amount;
    copy.Max.y = rect.Min.y;
    return copy;
}

static ImRect CutBottom(ImRect& rect, float amount)
{
    ImRect copy = rect;
    rect.Max.y -= amount;
    copy.Min.y = rect.Max.y;
    return copy;
}

static ImRect Left(const ImRect& rect, float amount)
{
    ImRect copy = rect;
    copy.Max.x = rect.Min.x + amount;
    return copy;
}

static ImRect Right(const ImRect& rect, float amount)
{
    ImRect copy = rect;
    copy.Min.x = rect.Max.x - amount;
    return copy;
}

static ImRect Top(const ImRect& rect, float amount)
{
    ImRect copy = rect;
    copy.Max.y = rect.Min.y + amount;
    return copy;
}

static ImRect Bottom(const ImRect& rect, float amount)
{
    ImRect copy = rect;
    copy.Min.y = rect.Max.y - amount;
    return copy;
}

void ShowTestWindow()
{
}

void update()
{
    FlushMessageQueue();
    {
        void* buffers[32];
        int32_t* sizes[32];

        int32_t n = 0;
        Span s;
        do {
            n = GetMessages(buffers, sizes, 32);
            for (int i = 0; i < n; i++) {
                s.Data = (uint8_t*)buffers[i];
                s.Length = (size_t)sizes[i];
                ProcessMessage(s);
                free(buffers[i]);
            }
        } while (n == 32);
    }

    int width = canvas_get_width();
    int height = canvas_get_height();
    SDL_SetWindowSize(g_window, width, height);

    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        //if (event.type == SDL_QUIT)
        //    done = true;
        //if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
        //    done = true;
    }

    SDL_GL_MakeCurrent(g_window, g_gl_context);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
    ShowTestWindow();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_window);
}

extern "C" int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) != 0) {
        printf("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    g_window = SDL_CreateWindow("Dear ImGui", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 512, SDL_WINDOW_OPENGL);

    g_gl_context = SDL_GL_CreateContext(g_window);
    
    {
        // Setup Dear ImGui binding
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context);
        ImGui_ImplOpenGL3_Init("#version 100");

        // Setup style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Load Fonts
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 23.0f);
        io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 18.0f);
        io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 26.0f);
        io.Fonts->AddFontFromFileTTF("data/xkcd-script.ttf", 32.0f);
        io.Fonts->AddFontDefault();

        imgui = ImGui::GetCurrentContext();
    }
    resizeCanvas();
    SDL_SetWindowSize(g_window, canvas_get_width(), canvas_get_height());

    Init(PersistentAlloc);
    emscripten_set_main_loop(update, 0, 1);
    return 0;
}