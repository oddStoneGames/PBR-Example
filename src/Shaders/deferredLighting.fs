#version 420 core
layout (location = 0) out vec4 FragmentColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoord;

struct PointLight
{
    vec3 position;
    vec3 color;
    float intensity;

    //Which Lighting Model To use.
    bool blinn;

    //Shadows
    bool shadows;
    // 0 - Hard Shadows
    // 1 - Soft Shadows
    // 2 - Faster Soft Shadows
    uint shadowType;
    float softShadowOffset;
    float fsoftShadowFactor;
    bool debugShadow;
    samplerCube shadowMap;
    float shadowFarPlane;

    //Attenuation Stuff
    float linear;
    float quadratic;
};

#define NR_OF_POINT_LIGHTS 2
uniform PointLight pointLight[NR_OF_POINT_LIGHTS];

uniform vec3 viewPos;

uniform float ambientStrength;
uniform bool ssaoEnabled;
uniform float specularStrength;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gEmission;
uniform sampler2D gMetallicRoughness;
uniform sampler2D ambientOcclusionMap;

//PBR
uniform bool pbrEnabled;
uniform bool physicallyCorrectAttenuation;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------

// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculation(PointLight light, vec3 FragPos)
{
    // Get vector between fragment position and light position
    vec3 fragToLight = FragPos - light.position;
    // Get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    
    float bias = 0.05f; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = 0.0f;

    if(light.shadowType == 0)
    {
        //Hard Shadows
        // use the fragment to light vector to sample from the depth map
        float closestDepth = texture(light.shadowMap, fragToLight).r;
        // it is currently in linear range between [0,1], let's re-transform it back to original depth value
        closestDepth *= light.shadowFarPlane;
        shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
    }
    else if(light.shadowType == 1)
    {
        //Soft Shadows(Percentage Close Filtering)
        float samples = 4.0f;
        float offset = light.softShadowOffset;
        for(float x = -offset; x < offset; x += offset / (samples * 0.5))
        {
            for(float y = -offset; y < offset; y += offset / (samples * 0.5))
            {
                for(float z = -offset; z < offset; z += offset / (samples * 0.5))
                {
                    float closestDepth = texture(light.shadowMap, fragToLight + vec3(x, y, z)).r; // use lightdir to lookup cubemap
                    closestDepth *= light.shadowFarPlane;   // Undo mapping [0;1]
                    if(currentDepth - bias > closestDepth)
                        shadow += 1.0;
                }
            }
        }
        shadow /= (samples * samples * samples);
    }
    else
    {
        //Fast Soft Shadows
        int samples = 20;
        float viewDistance = length(viewPos - FragPos);
        float diskRadius = (1.0 + (viewDistance / light.shadowFarPlane)) / light.fsoftShadowFactor;
        for(int i = 0; i < samples; ++i)
        {
            float closestDepth = texture(light.shadowMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
            closestDepth *= light.shadowFarPlane;   // undo mapping [0;1]
            if(currentDepth - bias > closestDepth)
                shadow += 1.0;
        }
        shadow /= float(samples);
    }

    return shadow;
}

vec3 CalculatePointLight(PointLight light, vec3 FragPos, vec3 Normal, vec3 viewDir, vec3 ambientColor, vec3 baseColor)
{
    vec3 LightPos = light.position;
    vec3 lightDir = normalize(LightPos - FragPos);
    float dist = length(LightPos - FragPos);
    float attenuation = 1.0f / (1.0f + light.linear * dist + light.quadratic * dist * dist);

    vec3 lightIntensifiedColor = light.intensity * light.color;

    //Multiply Ambience By Light Color.
    ambientColor *= light.color;

    float lightRelation = max(dot(lightDir, Normal), 0.0f);
    baseColor *= vec3(attenuation * lightRelation * lightIntensifiedColor);

    const float kPi = 3.14159265;
    const float kShininess = 33.0;

    vec3 specular = vec3(specularStrength);

    if(baseColor == 0.0f)
    {
        //Nullify Specular Component When There is no Diffuse Component.
        specular = vec3(0.0f);
    }
    else
    {
        if(light.blinn)
        {
           const float kEnergyConservation = ( 8.0 + kShininess ) / ( 8.0 * kPi );
           vec3 halfwayDir = normalize(lightDir + viewDir); 
           specular *= vec3(attenuation * kEnergyConservation * pow(max(dot(Normal, halfwayDir), 0.0), kShininess) * lightIntensifiedColor);
        }
        else
        {
           const float kEnergyConservation = ( 2.0 + kShininess ) / ( 2.0 * kPi );
           vec3 reflectDir = reflect(-lightDir, Normal);
           specular *= vec3(attenuation * kEnergyConservation * pow(max(dot(viewDir, reflectDir), 0.0), kShininess) * lightIntensifiedColor);
        }
    }

    float shadow = light.shadows ? ShadowCalculation(light, FragPos) : 0.0f;

    return ambientColor + (1.0f - shadow) * (baseColor + specular);
}

void main()
{   
    // Retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoord).rgb;
    vec3 Normal = texture(gNormal, TexCoord).rgb;
    vec3 baseColor = texture(gAlbedo, TexCoord).rgb;
    vec3 emissionColor = texture(gEmission, TexCoord).rgb;
    float AmbientOcclusion = ssaoEnabled ? texture(ambientOcclusionMap, TexCoord).r : 1.0f;

    // Get View Direction.
    vec3 viewDir  = normalize(viewPos - FragPos);

    vec3 lightingResult = vec3(0.0f);

    if(pbrEnabled)
    {
        //PBR Shading.
        
        //Change Base Color To Linear Space.
        baseColor = pow(baseColor, vec3(2.2));

        //Get Metallic & Roughness.
        float metallic = texture(gMetallicRoughness, TexCoord).r;
        float roughness = texture(gMetallicRoughness, TexCoord).g;

        vec3 R = reflect(-viewDir, Normal);

        // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
        // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
        vec3 F0 = vec3(0.01);
        F0 = mix(F0, baseColor, metallic);

        // reflectance equation
        vec3 Lo = vec3(0.0);
        for(int i = 0; i < NR_OF_POINT_LIGHTS; ++i) 
        {
            // calculate per-light radiance
            vec3 L = normalize(pointLight[i].position - FragPos);
            vec3 H = normalize(viewDir + L);
            float dist = length(pointLight[i].position - FragPos);
            float attenuation = physicallyCorrectAttenuation ? 1.0 / (dist * dist) : 1.0 / (pointLight[i].quadratic * dist * dist);
            vec3 radiance = pointLight[i].color * pointLight[i].intensity * attenuation;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(Normal, H, roughness);
            float G   = GeometrySmith(Normal, viewDir, L, roughness);
            vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
            
            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(Normal, viewDir), 0.0) * max(dot(Normal, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
            vec3 specular = numerator / denominator;
            
             // kS is equal to Fresnel
            vec3 kS = F;
            // for energy conservation, the diffuse and specular light can't
            // be above 1.0 (unless the surface emits light); to preserve this
            // relationship the diffuse component (kD) should equal 1.0 - kS.
            vec3 kD = vec3(1.0) - kS;
            // multiply kD by the inverse metalness such that only non-metals 
            // have diffuse lighting, or a linear blend if partly metal (pure metals
            // have no diffuse light).
            kD *= 1.0 - metallic;	                
                
            // scale light by NdotL
            float NdotL = max(dot(Normal, L), 0.0);        

            //Calculate Shadows if Enabled.
            float shadow = pointLight[i].shadows ? ShadowCalculation(pointLight[i], FragPos) : 0.0f;

            // add to outgoing radiance Lo
            Lo += (1.0 - shadow) * (kD * baseColor / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        }
    
        // ambient lighting (we now use IBL as the ambient term)
        vec3 F = fresnelSchlickRoughness(max(dot(Normal, viewDir), 0.0), F0, roughness);
    
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;
        
        vec3 irradiance = texture(irradianceMap, Normal).rgb;
        vec3 diffuse    = irradiance * baseColor;
        
        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(brdfLUT, vec2(max(dot(Normal, viewDir), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        vec3 ambient = (kD * diffuse + specular) * AmbientOcclusion;
        
        lightingResult = ambient + Lo;
    }
    else
    {
        //Normal Shading.
        vec3 ambientColor = ambientStrength * AmbientOcclusion * baseColor;
        for(int i = 0; i < NR_OF_POINT_LIGHTS; i++)
            lightingResult += CalculatePointLight(pointLight[i], FragPos, Normal, viewDir, ambientColor, baseColor);
    }

    //Final Fragment Color.
    vec3 color = lightingResult + emissionColor;

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    if(pointLight[0].debugShadow)
    {
        vec3 fragToLight = FragPos - pointLight[0].position;
        float currentDepth = length(fragToLight);
        float closestDepth = texture(pointLight[0].shadowMap, fragToLight).r;
        FragmentColor = vec4(vec3(closestDepth), 1.0f);
    }
    else if(pointLight[1].debugShadow)
    {
        vec3 fragToLight = FragPos - pointLight[1].position;
        float currentDepth = length(fragToLight);
        float closestDepth = texture(pointLight[1].shadowMap, fragToLight).r;
        FragmentColor = vec4(vec3(closestDepth), 1.0f);
    }
    else
        FragmentColor = vec4(color, 1.0f);

    //Output Brightness Color To be used By Bloom Pass.
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 2.0)
        BrightColor = vec4(color, 1.0f);
	else
		BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}