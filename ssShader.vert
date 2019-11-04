#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 fragTexCoord;

vec2 texCoords[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
	vec2(1.0, 0.0));
	
vec2 positions[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0),
	vec2(1.0, -1.0));

int verticesMapping[6]= int[](0,1,2,0,2,3);
	
void main() 
{
	int idx=verticesMapping[gl_VertexIndex];
    gl_Position = vec4(positions[idx], 0.0, 1.0);
	fragTexCoord=texCoords[idx];
}