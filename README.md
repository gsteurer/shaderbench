# Shaderbench
A desktop utility for writing toy fragment shaders in a similar manner to shadertoy.com written in vulkan with glfw. Shaderbench currently only supports glsl files. 

Usage:

`./build/bin/main` # defaults to shader.frag in CWD

`./build/bin/main path/filename` # a fragment shader written in glsl. shaderbench takes care of compilation to spirv.


### Todo
 * giant main.cpp file is hard to read
 * cmake or linux and windows makefiles
### Credits
[shadertoy](https://www.shadertoy.com/) 

[vulkan tutorial](https://vulkan-tutorial.com/Introduction)

