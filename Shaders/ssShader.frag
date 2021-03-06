#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout( set =0, binding = 0) uniform sampler2D inputTex;

void main()
{
	outColor += vec4( texture(inputTex, fragTexCoord+vec2(0.003, 0.003)).rgb, 1.0f);
	outColor += vec4( texture(inputTex, fragTexCoord+vec2(-0.003, 0.003)).rgb, 1.0f);
	outColor += vec4( texture(inputTex, fragTexCoord+vec2(-0.003, -0.003)).rgb, 1.0f);
	outColor += vec4( texture(inputTex, fragTexCoord+vec2(0.003, -0.003)).rgb, 1.0f);
	outColor += vec4( texture(inputTex, fragTexCoord).rgb, 1.0f);
	
	outColor/= 5.0;
}