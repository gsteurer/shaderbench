
#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(location = 0) out vec4 fragColor;
// yoinked from https://www.shadertoy.com/view/Md23DV
void main() {

vec3 color = vec3(0.0, 1.0, 1.0);
	float alpha = 1.0;
	
	vec4 pixel = vec4(color, alpha);
	fragColor = pixel;
}