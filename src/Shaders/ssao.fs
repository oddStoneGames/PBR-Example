#version 420 core
out float FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gLinearDepth;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[16];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 16;
uniform float radius = 0.1;
uniform float bias = 0.01;
uniform float strength = 4.0f;

// tile noise texture over screen based on screen dimensions divided by noise size
uniform vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0);

uniform mat4 view;
uniform mat4 projection;

void main()
{
    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoord).xyz;
    vec3 normal = vec3(normalize(view * vec4(texture(gNormal, TexCoord).rgb, 0.0f)));
    vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz);

    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;

    float fragPosDepth = (view * vec4(fragPos, 1.0f)).z;
    for(int i = 0; i < kernelSize; ++i)
    {
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 samplePos = view * vec4(fragPos + TBN * samples[i] * radius, 1.0f);

        vec4 offset = projection * samplePos; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // get depth value of kernel sample
        float sampleDepth = (view * vec4(texture(gPosition, offset.xy).xyz, 1.0f)).z;

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPosDepth - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;   
    }
    occlusion = 1.0 - (occlusion / kernelSize);
    
    FragColor = pow(occlusion, strength);
}