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

SHARED_ARGS = -O3 -s USE_SDL=2 -std=c++11
ASYNCIFY_ARGS = -s ASYNCIFY -s 'ASYNCIFY_IMPORTS=["paste", "copy"]'
LIBS = -lGL
LINKER_ARGS = $(SHARED_ARGS) -s USE_WEBGL2=1 -s WASM=1 -s FULL_ES3=1 -s USE_SDL=2 ${LIBS} $(ASYNCIFY_ARGS)
COMPILER_ARGS = $(SHARED_ARGS)

all: $(OUTPUT_JS) $(OUTPUT_HTML)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< -o $@ -c $(COMPILER_ARGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends

$(OUTPUT_HTML): imgui.html 
	cp $< $@

$(OUTPUT_JS): $(OBJ)
	@mkdir -p $(OUTPUT_DIR)
	$(CXX) $^ -o $(OUTPUT_JS) $(LINKER_ARGS) --preload-file data
	cp -r data $(OUTPUT_DIR)/data

clean_output:
	@rm $(OUTPUT_JS)

relink: clean_output $(OUTPUT_JS)


superclean: clean
	rm -rf $(BUILD_DIR)/imgui

clean:
	rm -rf $(BUILD_DIR)/src
	rm -rf $(OUTPUT_DIR)
