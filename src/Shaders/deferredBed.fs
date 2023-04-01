#version 420 core
in VS_OUT
{
    vec2 TexCoord;
    vec3 FragPos;
    vec3 Normal;
    mat3 TBN;
} fs_in;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gEmission;
layout (location = 4) out vec2 gMetallicRoughness;

uniform struct Material
{
    uint hasBCT;                                // Tells if There is A Base Color Texture For This Mesh.
    sampler2D baseColorTexture;                 // BCT

    uint hasMRT;                                // Tells if There is a metallicRoughness Texture.
    sampler2D metallicRoughnessTexture;         // Metallic Roughness Texture.
    float metallicFactor;                       // Metallic Factor Multiplied By The Sampling of the Blue Color of Metallic Roughness Texture.
    float roughnessFactor;                      // Roughness Factor Multiplied By The Sampling of the Green Color of Metallic Roughness Texture.

    uint hasET;                                 // Tells if There is A Emissive Texture for this Mesh.
    float emissionStrength;                     // The Strength Of The Emission Texture To Add Color Bleeding.
    sampler2D emissionTexture;                  // Emission Texture.

    uint hasNT;                                 // Tells if There is A Normal Texture for this Mesh.
    sampler2D normalTexture;                    // Normal Texture.
}material;

void main()
{
    //Store The Fragment Position in the First gBuffer Texture.
    gPosition = fs_in.FragPos;

    //Store The Fragment Normal in the Second gBuffer Texture.
    vec3 normal = material.hasNT > 0 ? normalize(fs_in.TBN * (texture(material.normalTexture, fs_in.TexCoord).rgb * 2.0 - 1.0)) : normalize(fs_in.Normal);
    gNormal = normal;

    //Get Emission Color.
    vec3 emissionColor = material.emissionStrength * material.hasET * texture2D(material.emissionTexture, fs_in.TexCoord).rgb;

    //Get Base Color.
    vec3 baseColor = material.hasBCT * texture2D(material.baseColorTexture, fs_in.TexCoord).rgb;

    //Don't Have Base Color where there is Emission.
    if(emissionColor.r > 0.1f && material.hasET == 1) baseColor = vec3(0.0f);

    //Store The Fragment Albedo Data in the Third gBuffer Texture.
    gAlbedo = baseColor;

    //Store The Fragment Emission Data in the Fourth gBuffer Texture.
    gEmission = emissionColor;

    //Get Metallic Roughness Value
    vec2 metallicRoughness = vec2(1.0f);
    //Multiply Roughness & Roughness Factor & Metallicness By Metallic Factor.     
    metallicRoughness.r *= clamp(material.metallicFactor, 0.0, 1.0);
    metallicRoughness.g *= clamp(material.roughnessFactor, 0.0, 1.0);
    if(material.hasMRT > 0)
        metallicRoughness *= texture2D(material.metallicRoughnessTexture, fs_in.TexCoord).bg;
    
    //Store The Fragment Metallic Roughness Data in the Fifth gBuffer Texture.
    gMetallicRoughness = metallicRoughness;
}