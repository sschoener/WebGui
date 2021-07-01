CXX = emcc
OUTPUT_DIR = build
OUTPUT = imgui.js
IMGUI_DIR:=../imgui

SOURCES = main.cpp
SOURCES += imgui_impl_glfw.cpp imgui_impl_opengl3.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/imgui_tables.cpp

LIBS = -lGL
WEBGL_VER = -s USE_WEBGL2=1 -s USE_GLFW=3 -s FULL_ES3=1
#WEBGL_VER = USE_GLFW=2
USE_WASM = -s WASM=1

all: $(SOURCES) $(OUTPUT)

$(OUTPUT): $(SOURCES) 
	mkdir -p $(OUTPUT_DIR)
	$(CXX)  $(SOURCES) -std=c++11 -o $(OUTPUT_DIR)/$(OUTPUT) $(LIBS) $(WEBGL_VER) -O2 --preload-file data $(USE_WASM) -I$(IMGUI_DIR)
	cp imgui.html $(OUTPUT_DIR)/imgui.html
	cp -r data $(OUTPUT_DIR)/data

clean:
	rm -rf $(OUTPUT_DIR)
