# WebGui
WebGui is an example showing how to use the IMGUI user interface in a web browser. This example uses Emscripted to compile c++ into Web Assembly (WASM) binaries that can be run as an application in the browser.
This repository uses a submodule for imgui, so please use `git submodule update --init --recursive` after cloning.

This repository builds on @jnmaloney's [WebGui](https://github.com/jnmaloney/WebGui). There are a few distinguishing factors:
 * This example uses SDL2 instead of GLFW3. According to the emscripten website, SDL2 enjoyes better support than GLFW.
 * This version supports copy and paste. Turns out that the most difficult part of getting a useful web-app running is getting copy and paste to work. I'm using [this approach](https://github.com/pthom/hello_imgui/issues/3#issuecomment-873401502). Not sure how portable that is, but it works for me *shrug*.

# Building the application
Make sure that you have emscripten installed, as well as SDL2, Freetype, and OpenGL3. Then just run `make`.