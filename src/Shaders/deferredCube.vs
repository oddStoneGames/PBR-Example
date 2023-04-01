#version 420 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoord;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 Normal;
    mat3 TBN;
} vs_out;

uniform vec3 viewPos;
uniform mat4 model;

layout(std140, binding = 0)uniform Matrices
{
    mat4 viewProjection;
};

void main()
{
    vs_out.TexCoord = texCoord;
    vec4 worldPos      = model * vec4(pos, 1.0);
    vs_out.FragPos     = vec3(worldPos);
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    vs_out.TBN = mat3(T, B, N);
    mat3 TBN = transpose(vs_out.TBN);
    vs_out.Normal = N;
    vs_out.TangentViewPos = TBN * viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;
    gl_Position = viewProjection * model * vec4(pos, 1.0);
}