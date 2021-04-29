#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 color;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(color, 1.0);
}
