#version 420 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texCoord;

out vec3 Normal;

void main()
{
    gl_Position = vec4(pos, 1.0);
    Normal      = normalize(normal);
}