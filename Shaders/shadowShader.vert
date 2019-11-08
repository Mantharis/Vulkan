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


void main() 
{
    gl_Position = pushConsts.proj * pushConsts.view * pushConsts.model * vec4(inPosition, 1.0);
}


/*
vec2 positions[4] = vec2[](
    vec2(-0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
	vec2(0.5, -0.5));

int verticesMapping[6]= int[](0,1,2,0,2,3);
	
void main() 
{
	int idx=verticesMapping[gl_VertexIndex];
    gl_Position = vec4(positions[idx], 0.1, 1.0);
}
*/