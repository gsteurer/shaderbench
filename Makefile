INCLUDES=-I$${HOME}/Code/c_workbench/external/glfw/include -I$${HOME}/Code/cpp_workbench/external/glm -I$${HOME}/SDK/VulkanSDK/1.2.182.0/macOS/include  
LIB_LOCATIONS=-L$${HOME}/Code/c_workbench/external/glfw/src -L$${HOME}/SDK/VulkanSDK/1.2.182.0/macOS/lib
LIBS=-lglfw3 -lpthread -lvulkan

FRAMEWORKS=-framework Cocoa -framework IOKit -framework CoreAudio

CC=clang++ -g -O0 -std=c++17 
CPPFLAGS=-Wall -Wextra $(INCLUDES) -DENABLE_VALIDATION_LAYERS

.PHONY: shaders

default: dirs
	$(MAKE) shaders main

dirs:
	mkdir -p build/obj build/bin build/shaders

clean:
	rm -rf build *.spv

shaders: dirs
	glslangValidator -V shader.vert -o build/shaders/shader.vert.spv
	glslangValidator -V shader.frag -o build/shaders/shader.frag.spv

build/obj/main.o: main.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $<

main: build/obj/main.o
	$(CC) $(CPPFLAGS) $(LIB_LOCATIONS) $(LIBS) $(FRAMEWORKS) -o build/bin/$@ $^