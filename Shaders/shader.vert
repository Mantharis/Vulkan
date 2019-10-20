#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConsts 
{
	mat4 proj;
	mat4 view;
	mat4 model;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec3 fragNormal;

void main() 
{
    gl_Position = pushConsts.proj * pushConsts.view * pushConsts.model * vec4(inPosition, 1.0);
	fragPosition= (pushConsts.model * vec4(inPosition, 1.0)).xyz;
	fragNormal = inNormal;
	
	fragColor = inColor;
	fragTexCoord=inTexCoord;
}