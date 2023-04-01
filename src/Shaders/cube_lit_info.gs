#version 420 core
layout(triangles) in;
layout(line_strip, max_vertices = 12) out;

in vec3 Normal[];
in vec3 FragPos[];

out vec3 FragColor;

uniform vec3 lightPosition[2];
uniform mat4 view;
uniform mat4 projection;

void ShowFragmentToLightDirection(int index, vec3 LightPos)
{
    gl_Position = projection * vec4(FragPos[index], 1.0f);
    FragColor = vec3(1.0f);
    EmitVertex();

    vec3 lightDir = normalize(LightPos - FragPos[index]);
    float lightRelation = max(dot(lightDir, normalize(Normal[index])), 0);
    gl_Position = projection * vec4(FragPos[index] + LightPos - FragPos[index], 1.0f);
    FragColor = vec3(lightRelation);
    EmitVertex();

    EndPrimitive();
}

void main()
{
    vec3 LightPos = vec3(view * vec4(lightPosition[0], 1.0f));
    ShowFragmentToLightDirection(0, LightPos);
    ShowFragmentToLightDirection(1, LightPos);
    ShowFragmentToLightDirection(2, LightPos);

    vec3 LightPos1 = vec3(view * vec4(lightPosition[1], 1.0f));
    ShowFragmentToLightDirection(0, LightPos1);
    ShowFragmentToLightDirection(1, LightPos1);
    ShowFragmentToLightDirection(2, LightPos1);
}