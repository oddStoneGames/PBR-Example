#version 420 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoord;

out VS_OUT
{
    vec2 TexCoord;
    vec3 FragPos;
    vec3 Normal;
    mat3 TBN;
} vs_out;

uniform mat4 model;

layout(std140, binding = 0)uniform Matrices
{
    mat4 viewProjection;
};

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * normal);
    vec3 T = normalize(normalMatrix * tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vs_out.TexCoord     = mat2(0.0, -1.0, 1.0, 0.0) * texCoord;
    vec4 worldPos       = model * vec4(pos, 1.0);
    vs_out.FragPos      = vec3(worldPos);
    vs_out.Normal       = N;
    vs_out.TBN          = mat3(T, B, N);
    
    gl_Position     = viewProjection * worldPos;
}