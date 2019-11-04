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
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout( set =1, binding = 0) uniform SceneUBO
{
	vec3 cameraPos;
	vec3 lightPos;
	vec3 lightColor;
	uint lightCount;
} scene;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out vec3 fragBitangent;
void main() 
{
    gl_Position = pushConsts.proj * pushConsts.view * pushConsts.model * vec4(inPosition, 1.0);
	fragPosition= (pushConsts.view * pushConsts.model * vec4(inPosition, 1.0)).xyz;
	
	fragNormal = normalize(pushConsts.view * pushConsts.model * vec4(inNormal, 0.0f)).xyz;
    fragTangent = normalize((pushConsts.view * pushConsts.model * vec4(normalize(inTangent), 0.0))).xyz;
    fragBitangent = normalize((pushConsts.view * pushConsts.model * vec4(normalize(inBitangent), 0.0))).xyz;

	fragColor = inColor;
	fragTexCoord=inTexCoord;
}