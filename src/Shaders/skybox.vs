#version 420 core
layout(location = 0) in vec3 pos;

out vec3 TexCoord;

uniform mat4 viewProjection;

void main()
{
    //TexCoord = vec3(pos.x, -pos.y, pos.z);
    TexCoord = pos;
    vec4 position = viewProjection * vec4(pos, 1.0);
    gl_Position = position.xyww;
}