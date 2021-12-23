# Shaderbench
A desktop utility for writing toy fragment shaders in a similar manner to shadertoy.com written in vulkan with glfw. Shaderbench currently only supports glsl files. A great tutorial for writing toy fragment shaders [exists here](https://www.shadertoy.com/view/Md23DV). Esentially, in shaderbench, a fragment shader is a program that runs on your GPU for each pixel in the window. 

### Usage:

`./build/bin/main` # defaults to shader.frag in CWD

`./build/bin/main path/filename` # a fragment shader written in glsl. shaderbench takes care of compilation to spirv.


### Todo
 * giant main.cpp file is hard to read
 * cmake or linux and windows makefiles
 * fix coordinate system - I think vulkan is inverted y axis
### Dependencies
 * [glm](https://github.com/g-truc/glm)
 * [glfw](https://github.com/glfw/glfw) 
 * [vulkan sdk](https://vulkan.lunarg.com/)

### Build
Clone the dependencies to a path on your system. You will need to build glfw. Refer to the makefile for directory locations.

### Credits
[shadertoy](https://www.shadertoy.com/) 

[vulkan tutorial](https://vulkan-tutorial.com/Introduction)

