#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(push_constant) uniform PushConsts 
{
	mat4 viewProj;
} pushConsts;


struct Particle
{
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 velocity;
	
	vec2 t0;
	vec2 t1;
	vec2 t2;
	
	float mass;
};

layout(std430, binding = 0) buffer Pos 
{
   Particle particle[];
};

layout (location = 0) out vec2 outTexcoord;

void main() 
{
	int particleIdx = gl_VertexIndex /3;
	int vIdx= gl_VertexIndex % 3;
	
	vec3 pos;
	if (gl_VertexIndex % 3 ==0)
	{
		pos =particle[particleIdx].v0;
		outTexcoord = particle[particleIdx].t0;
	}
	else if (gl_VertexIndex % 3 ==1)
	{
		pos = particle[particleIdx].v1;
		outTexcoord = particle[particleIdx].t1;
	}
	else if (gl_VertexIndex % 3 ==2)
	{
		pos = particle[particleIdx].v2;
		outTexcoord = particle[particleIdx].t2;
	}
	
	gl_Position = pushConsts.viewProj * vec4(pos, 1.0);
}