#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec3 fragBitangent;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outPos;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outMetallicRoughnessAo;

layout( set =0, binding = 0) uniform MaterialUBO 
{
    vec3 diffuse;
	vec3 ambient;
	vec3 specular;
	float shininess;
} material;


layout( set =0, binding = 5) uniform sampler2D aoMap;
layout( set =0, binding = 4) uniform sampler2D roughnessMap;
layout( set =0, binding = 3) uniform sampler2D normalMap;
layout( set =0, binding = 2) uniform sampler2D metallicMap;
layout( set =0, binding = 1) uniform sampler2D albedoMap;

 
void main()
{		
	outAlbedo.rgb     = pow(texture(albedoMap, fragTexCoord).rgb, vec3(2.2));
	outPos.rgb = fragPosition;
	
	mat3 TBN=mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragNormal));
	
	outNormal.rgb = TBN* normalize(  texture(normalMap, fragTexCoord).rgb*2.0 - vec3(1.0));
	outNormal.a = 0.0;
	
	outMetallicRoughnessAo.rgb = vec3(texture(metallicMap, fragTexCoord).r, texture(roughnessMap, fragTexCoord).r, texture(aoMap, fragTexCoord).r);
}  