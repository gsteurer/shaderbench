#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec3 iResolution;
    float iTime;
    vec4 iMouse;
} ubo;

layout(location = 0) out vec4 outColor;
// yoinked from https://www.shadertoy.com/view/Md23DV
void main() {

	vec2 r = vec2(gl_FragCoord.x / ubo.iResolution.x,
				  gl_FragCoord.y / ubo.iResolution.y);
	
	vec3 color1 = vec3(1.0, 0.0, 0.0);
	vec3 color2 = vec3(0.0, 1.0, 0.0);
	vec3 color3 = vec3(0.0, 0.0, 1.0);
	vec3 pixel;

	if( r.x < 1.0/3.0) {
		pixel = color1;
	} else if( r.x < 2.0/3.0 ) {
		pixel = color2;
	} else {
		pixel = color3;
	}
			
	outColor = vec4(pixel, 1.0);
    //outColor = vec4(abs(sin(ubo.iTime))/2, abs(sin(ubo.iTime))/4, abs(sin(ubo.iTime))/8, 1.0);
    
}