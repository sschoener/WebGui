CXX = emcc
OUTPUT_DIR = build
BUILD_DIR = tmp_build
IMGUI_BUILD_DIR = imgui_build
OUTPUT_JS = ${OUTPUT_DIR}/imgui.js
OUTPUT_HTML = ${OUTPUT_DIR}/imgui.html
IMGUI_DIR:=imgui

LOCAL_SOURCES = main.cpp imgui_impl_sdl.cpp imgui_impl_opengl3.cpp
LOCAL_OBJ = $(addprefix $(BUILD_DIR)/,$(LOCAL_SOURCES:%.cpp=%.o))
IMGUI_SOURCES = imgui.cpp imgui_draw.cpp imgui_demo.cpp imgui_widgets.cpp imgui_tables.cpp
IMGUI_OBJ = $(addprefix $(IMGUI_BUILD_DIR)/,$(IMGUI_SOURCES:%.cpp=%.o))

SHARED_ARGS = -O3 -s USE_SDL=2 -std=c++11
ASYNCIFY_ARGS = -s ASYNCIFY -s 'ASYNCIFY_IMPORTS=["paste", "copy"]'
LIBS = -lGL
LINKER_ARGS = $(SHARED_ARGS) -s USE_WEBGL2=1 -s WASM=1 -s FULL_ES3=1 -s USE_SDL=2 ${LIBS} $(ASYNCIFY_ARGS)
COMPILER_ARGS = $(SHARED_ARGS) $(ASYNCIFY_ARGS)

all: $(OUTPUT_JS) $(OUTPUT_HTML)

$(IMGUI_BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< -o $@ -c $(COMPILER_ARGS) -I$(IMGUI_DIR)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< -o $@ -c $(COMPILER_ARGS) -I$(IMGUI_DIR)

$(OUTPUT_HTML): imgui.html 
	cp $< $@

$(OUTPUT_JS): $(IMGUI_OBJ) $(LOCAL_OBJ)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $^ -o $(OUTPUT_JS) $(LINKER_ARGS) --preload-file data
	cp -r data $(OUTPUT_DIR)/data

clean_output:
	@rm $(OUTPUT_JS)

relink: clean_output $(OUTPUT_JS)


superclean: clean
	rm -rf $(IMGUI_BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(OUTPUT_DIR)
