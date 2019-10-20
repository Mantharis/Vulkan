#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout( set =0, binding = 0) uniform MaterialUBO 
{
    vec3 diffuse;
	vec3 ambient;
	vec3 specular;
	float shininess;
} material;

layout( set =0, binding = 2) uniform sampler2D texSampler2;
layout( set =0, binding = 1) uniform sampler2D texSampler;

layout( set =1, binding = 0) uniform SceneUBO
{
	vec3 cameraPos;
	vec3 lightPos;
	vec3 lightColor;
	uint lightCount;
} scene;
 
void main()
{
	vec3 l=normalize(scene.lightPos-fragPosition);
	vec3 n= normalize(fragNormal); 
	vec3 d= scene.lightColor;
	
	vec3 e=normalize(scene.cameraPos-fragPosition);
	
	vec3 diffuse= d * max(0.0, dot(n, l)) * texture(texSampler, fragTexCoord).rgb * material.diffuse;
  
	vec3 r = normalize(reflect(-l,n)); 
	//vec3 specular= d * pow(max(dot(r,e), 0.0), material.shininess) * material.specular;
	vec3 specular= vec3(0.0f);
  
	vec3 ambient=material.ambient * texture(texSampler2, fragTexCoord).rgb ;

	outColor=vec4( (specular+diffuse+ambient) * fragColor, 1.0f);
}