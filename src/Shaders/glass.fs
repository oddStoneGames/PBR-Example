#version 420 core
in VS_OUT
{
    vec2 TexCoord;
    vec3 Normal;
    mat3 TBN;
} fs_in;

layout (location = 0) out vec4 FragmentColor;
layout (location = 1) out vec4 BrightColor;

uniform struct Material
{
    uint hasBCT;                                // Tells if There is A Base Color Texture For This Mesh.
    sampler2D baseColorTexture;                 // BCT

    uint hasMRT;                                // Tells if There is a metallicRoughness Texture.
    sampler2D metallicRoughnessTexture;         // Metallic Roughness Texture.
    float metallicFactor;                       // Metallic Factor Multiplied By The Sampling of the Blue Color of Metallic Roughness Texture.
    float roughnessFactor;                      // Roughness Factor Multiplied By The Sampling of the Green Color of Metallic Roughness Texture.

    uint hasET;                                 // Tells if There is A Emissive Texture for this Mesh.
    sampler2D emissionTexture;                  // Emission Texture.

    uint hasNT;                                 // Tells if There is A Normal Texture for this Mesh.
    sampler2D normalTexture;                    // Normal Texture.
}material;

void main()
{
    //Store The Fragment Normal in the Second gBuffer Texture.
    vec3 normal = material.hasNT > 0 ? normalize(fs_in.TBN * (texture(material.normalTexture, fs_in.TexCoord).rgb * 2.0 - 1.0)) : normalize(fs_in.Normal);

    //Get Base Color.
    vec4 baseColor = material.hasBCT * texture2D(material.baseColorTexture, fs_in.TexCoord);

    FragmentColor = baseColor;

    float brightness = dot(FragmentColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = FragmentColor;
	else
		BrightColor = vec4(0.0, 0.0, 0.0, FragmentColor.a);
}