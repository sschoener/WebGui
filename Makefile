CXX = emcc
OUTPUT_DIR = build
BUILD_DIR = build_tmp
SRC_DIR = src
IMGUI_DIR:=imgui
OUTPUT_JS = ${OUTPUT_DIR}/imgui.js
OUTPUT_HTML = ${OUTPUT_DIR}/imgui.html

LOCAL_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
IMGUI_SOURCES = $(wildcard $(IMGUI_DIR)/*.cpp)

SOURCES = $(LOCAL_SOURCES) $(IMGUI_SOURCES) $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJ = $(addprefix $(BUILD_DIR)/,$(SOURCES:%.cpp=%.o))

SHARED_ARGS = -s USE_SDL=2 -std=c++11
ASYNCIFY_ARGS = -s ASYNCIFY -s 'ASYNCIFY_IMPORTS=["paste", "copy"]'
EXPORTED_FUNCTIONS = -s EXPORTED_FUNCTIONS='["_main"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
LIBS = -lGL
LINKER_ARGS = -s USE_WEBGL2=1 -s WASM=1 -s FULL_ES3=1 -s USE_SDL=2 ${LIBS} $(ASYNCIFY_ARGS) $(EXPORTED_FUNCTIONS)
COMPILER_ARGS =

results: $(OUTPUT_JS) $(OUTPUT_HTML)

all: SHARED_ARGS += -O3
all: results

debug: SHARED_ARGS += -g
debug: LINKER_ARGS += -s ASSERTIONS=1
debug: results

sanit: SHARED_ARGS += -fsanitize=undefined
# sanit: LINKER_ARGS += -s ALLOW_MEMORY_GROWTH -s INITIAL_MEMORY=536870912
sanit: results

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< -o $@ -c $(SHARED_ARGS) $(COMPILER_ARGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends

$(OUTPUT_HTML): imgui.html 
	cp $< $@

$(OUTPUT_JS): $(OBJ)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $^ -o $(OUTPUT_JS) $(SHARED_ARGS) $(LINKER_ARGS) --preload-file data
	cp -r data $(OUTPUT_DIR)/data

clean_output:
	@rm $(OUTPUT_JS)

relink: clean_output $(OUTPUT_JS)


superclean: clean
	rm -rf $(BUILD_DIR)/imgui

clean:
	rm -rf $(BUILD_DIR)/src
	rm -rf $(OUTPUT_DIR)
