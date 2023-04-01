#version 420 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = texCoord;
    gl_Position = vec4(pos, 1.0);
}