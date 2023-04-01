#version 420 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 viewModel;

layout(std140, binding = 0)uniform Matrices
{
    mat4 viewProjection;
};

void main()
{
    gl_Position = viewProjection * model * vec4(pos, 1.0);
    FragPos     = vec3(viewModel * vec4(pos, 1.0));
    Normal      = mat3(transpose(inverse(viewModel))) * normal;
}