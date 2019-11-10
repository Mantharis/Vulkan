#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout( set =0, binding = 5) uniform LightParams 
{
    mat4 viewProjMat;
	vec3 viewSpacePos;
	vec3 color;
	
} lightParams;

layout(push_constant) uniform PushConsts 
{
	mat4 viewProjMat;
	vec3 viewSpacePos;
	vec3 color;
} lightPushConsts;


layout( set =0, binding = 4) uniform sampler2D depthMap;
layout( set =0, binding = 3) uniform sampler2D metalicRoughnessAoMap;
layout( set =0, binding = 2) uniform sampler2D normalMap;
layout( set =0, binding = 1) uniform sampler2D albedoMap;
layout( set =0, binding = 0) uniform sampler2D fragPosMap;

 const float PI = 3.14159265359;
  
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}


void main()
{	
	vec3 cameraPos=vec3(0,0,0); //camera pos is at origin in view space
	vec3 N = texture(normalMap, fragTexCoord).rgb;
	
	vec3 albedo     = texture(albedoMap, fragTexCoord).rgb;
	
    float metallic  = texture(metalicRoughnessAoMap, fragTexCoord).r;
    float roughness = texture(metalicRoughnessAoMap, fragTexCoord).g;
    float ao        = texture(metalicRoughnessAoMap, fragTexCoord).b;
	
	vec3 lightPos=lightPushConsts.viewSpacePos;
	
	vec3 fragPosition= texture(fragPosMap, fragTexCoord).rgb;
	
	float distance=length(lightPos - fragPosition);
	
	vec3 L= normalize(lightPos-fragPosition);
	vec3 V=normalize(cameraPos-fragPosition);


    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);


    vec3 H = normalize(V + L);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightPushConsts.color * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = numerator / max(denominator, 0.001);  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
      
  
    //vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = /*ambient +*/ Lo;

	vec4 lightSpaceFragPos= lightPushConsts.viewProjMat * vec4(fragPosition, 1.0);
	lightSpaceFragPos = lightSpaceFragPos/lightSpaceFragPos.w;
	
	float lightSpaceFragDistance= lightSpaceFragPos.z;

	vec2 shadowMapTexCoord = vec2(0.5)+lightSpaceFragPos.xy*0.5;
	float shadowMapDepth = texture(depthMap, shadowMapTexCoord).r;
	
    color = color / (color + vec3(1.0));
	
	if (shadowMapDepth+0.0001f < lightSpaceFragDistance)
	{
		//color= vec3(0.0);
		color = vec3(0.3,0,0);
	}


    color = pow(color, vec3(1.0/2.2));  
   
    outColor = vec4(color, 1.0);
}  