#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fEyePos;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
//shadow
uniform sampler2D shadowMap;
//point lights
struct PointLight {    
    vec3 position;
    vec3 direction;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
};  
#define NR_POINT_LIGHTS 1  
uniform PointLight pointLights[NR_POINT_LIGHTS];

//spot lights
struct SpotLight {    
    vec3 position;
    vec3 direction;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;
};  
#define NR_SPOT_LIGHTS 1  
uniform SpotLight spotLights[NR_SPOT_LIGHTS];

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shadow;

void computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}

float computeFog(){
    float fogDensity=0.05f;
    float fragmentDistance = length(fEyePos);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

    return clamp(fogFactor, 0.0f, 1.0f);
}

float computeShadow(){
	// perform perspective divide
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;
	// Check whether current frag pos is in shadow
	float bias = max(0.05f * (1.0f - dot(fNormal, lightDir)), 0.005f);
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

	return shadow;
}

vec3 computeSpotLight(SpotLight light, vec3 normal, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    if(theta > light.outerCutOff){
        //TO CHANGE THIS-move outside, func param
        vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
        vec3 viewDir = normalize(- fPosEye.xyz);
    
        // diffuse shading
        float diff = max(dot(normal, lightDir), 0.0);
        // specular shading
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularStrength);
        // attenuation
        float distance    = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			         light.quadratic * (distance * distance));    
        // combine results
        vec3 ambient  = light.ambient  * vec3(texture(diffuseTexture, fTexCoords));
        vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuseTexture, fTexCoords));
        vec3 specular = light.specular * spec * vec3(texture(specularTexture, fTexCoords));
        ambient  *= attenuation;
        diffuse  *= attenuation;
        specular *= attenuation;
        diffuse  *= intensity;
        specular *= intensity;
        return (ambient + diffuse + specular);
    }else{
        return vec3(0.0f,0.0f,0.0f);
    }
}

vec3 computePointLight(PointLight light, vec3 normal, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    //TO CHANGE THIS-move outside, func param
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 viewDir = normalize(- fPosEye.xyz);
    
    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularStrength);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuseTexture, fTexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuseTexture, fTexCoords));
    vec3 specular = light.specular * spec * vec3(texture(specularTexture, fTexCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}



void main() 
{
    computeDirLight();

    //compute final vertex color
    vec4 colorFromTexture = texture(diffuseTexture, fTexCoords);
    if(colorFromTexture.a<0.1)
        discard;
    else{
        shadow = computeShadow();

        vec3 color = min((ambient + (1.0f - shadow)*diffuse) * colorFromTexture.rgb + (1.0f - shadow)*specular * texture(specularTexture, fTexCoords).rgb, 1.0f);
        //vec3 color = min((ambient + diffuse) * colorFromTexture.rgb + specular * texture(specularTexture, fTexCoords).rgb, 1.0f);
        
        for(int i = 0; i < NR_SPOT_LIGHTS; i++)
            color += computeSpotLight(spotLights[i], fNormal, fPosition);

        //for(int i = 0; i < NR_POINT_LIGHTS; i++)
            //color += computePointLight(pointLights[i], fNormal, fPosition);
            
        float fogFactor = computeFog();
        vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
        fColor = fogColor * (1 - fogFactor) + vec4(color,1.0f) * fogFactor;

         

        //without fog
        //fColor = vec4(color, 1.0f);
    }
    
}
