#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 eyeDir_tangentSpace;
layout(location = 5) in vec3 lightDir_tangentSpace;
layout(location = 6) in vec3 lightColor;

layout(location = 0) out vec4 outColor;

layout( set =0, binding = 0) uniform MaterialUBO 
{
    vec3 diffuse;
	vec3 ambient;
	vec3 specular;
	float shininess;
} material;


layout( set =0, binding = 4) uniform sampler2D specularExpSampler;
layout( set =0, binding = 3) uniform sampler2D normalSampler;
layout( set =0, binding = 2) uniform sampler2D specularSampler;
layout( set =0, binding = 1) uniform sampler2D diffuseSampler;

 
void main()
{
	vec3 cameraPos=vec3(0,0,0); //camera pos is at origin in view space
	vec3 lightPos=cameraPos;

	vec3 l= normalize(lightPos-fragPosition);
	
	vec3 n = normalize(  texture(normalSampler, fragTexCoord).rgb*2.0 - 1.0);

	vec3 d= lightColor;
	
	vec3 e=normalize(cameraPos-fragPosition);
	
	vec3 diffuse= d * max(0.0, dot(n, l)) * texture(diffuseSampler, fragTexCoord).rgb * material.diffuse;
	
	vec3 r = normalize(reflect(-l,n)); 

	

	vec3 specularExp = texture(specularExpSampler, fragTexCoord).rgb * material.shininess;

	vec3 specular= d * pow( vec3(max(dot(r,e), 0.0)), specularExp) * material.specular * texture(specularSampler, fragTexCoord).rgb;
	
	vec3 ambient=material.ambient * texture(specularSampler, fragTexCoord).rgb ;
	
	outColor=vec4( (specular+diffuse+ambient) * fragColor, 1.0f);
}