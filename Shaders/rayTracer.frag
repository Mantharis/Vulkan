#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D inputTex;

void main()
{
	outColor = vec4( texture(inputTex, fragTexCoord).rgb, 1.0f);
	//outColor = vec4(0.5, 1.0, 0.2, 1.0);
}