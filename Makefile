INCLUDES=-I$${HOME}/Code/c_workbench/external/glfw/include -I$${HOME}/Code/cpp_workbench/external/glm -I$${HOME}/SDK/Vulkan/1.3.261.1/macOS/include
LIB_LOCATIONS=-L$${HOME}/Code/c_workbench/external/glfw/build/src -L$${HOME}/SDK/Vulkan/1.3.261.1/macOS/lib
LIBS=-lglfw3 -lpthread -lvulkan -lshaderc_combined

FRAMEWORKS=-framework Cocoa -framework IOKit -framework CoreAudio

CC=clang++ -g -O0 -std=c++17 
CPPFLAGS=-Wall -Wextra $(INCLUDES) -DENABLE_VALIDATION_LAYERS

.PHONY: shaders

default: dirs
	$(MAKE) main

dirs:
	mkdir -p build/obj build/bin build/shaders

clean:
	rm -rf build *.spv

shaders: dirs
	glslangValidator -V shader.frag -o build/shaders/shader.frag.spv

build/obj/main.o: main.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $<

main: build/obj/main.o
	$(CC) $(CPPFLAGS) $(LIB_LOCATIONS) $(LIBS) $(FRAMEWORKS) -o build/bin/$@ $^