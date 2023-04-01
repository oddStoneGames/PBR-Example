#version 420 core
in VS_OUT {
    vec3 FragPos;
    vec2 TexCoord;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 Normal;
    mat3 TBN;
} fs_in;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gEmission;
layout (location = 4) out vec2 gMetallicRoughness;

uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D displacement;
uniform float roughness;
uniform float metallicness;
uniform float displacement_factor;
uniform int min_samples;
uniform int max_samples;

vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    float numLayers = mix(float(max_samples), float(min_samples), abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * displacement_factor;
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(displacement, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(displacement, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(displacement, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main()
{
    //Store The Fragment Position in the First gBuffer Texture.
    gPosition = fs_in.FragPos;

    // offset texture coordinates with Parallax Mapping
    vec3 tangentViewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoord;
    texCoords = ParallaxOcclusionMapping(fs_in.TexCoord, tangentViewDir);

    // obtain normal from normal map
    vec3 norm = texture(normal, texCoords).rgb;
    norm = normalize(fs_in.TBN * (norm * 2.0 - 1.0));
    //Store The Fragment Normal in the Second gBuffer Texture.
    gNormal = norm;

    //Get Base Color.
    vec3 baseColor = texture(diffuse, texCoords).rgb;
    //Store The Fragment Albedo Data in the Third gBuffer Texture.
    gAlbedo = baseColor;

    //Store The Fragment Emission Data in the Fourth gBuffer Texture.
    gEmission = vec3(0.0f);

    //Get Metallic Roughness Value
    vec2 metallicRoughness = vec2(1.0f);
    //Multiply Roughness & Roughness Factor & Metallicness By Metallic Factor.
    metallicRoughness.r *= clamp(metallicness, 0.0, 1.0);
    metallicRoughness.g *= clamp(roughness, 0.0, 1.0);

    //Store The Fragment Metallic Roughness Data in the Fifth gBuffer Texture.
    gMetallicRoughness = metallicRoughness;
}