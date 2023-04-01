#version 420 core
layout(points) in;
layout(line_strip, max_vertices = 6) out;

out GS_OUT
{
    vec3 FragColor;
}gs_out;

uniform float size;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 originModel;

void CreateAxes(mat3 normalMatrix)
{
    //Get Origin.
    vec4 origin = view * originModel * gl_in[0].gl_Position;

    //Create X Axis
    vec4 xAxis = vec4(normalize(normalMatrix*vec3(1.0f, 0.0f, 0.0f)), 0.0f);
    gl_Position = projection * origin;    
    gs_out.FragColor = vec3(1.0f, 0.0f, 0.0f);
    EmitVertex();
    gl_Position = projection * (origin + size * xAxis);
    gs_out.FragColor = vec3(1.0f, 0.0f, 0.0f);
    EmitVertex();

    //Create Y Axis
    vec4 yAxis = vec4(normalize(normalMatrix*vec3(0.0f, 1.0f, 0.0f)), 0.0f);
    gl_Position = projection * origin;
    gs_out.FragColor = vec3(0.0f, 1.0f, 0.0f);
    EmitVertex();
    gl_Position = projection * (origin + size * yAxis);
    gs_out.FragColor = vec3(0.0f, 1.0f, 0.0f);
    EmitVertex();

    //Create Z Axis
    vec4 zAxis = vec4(normalize(normalMatrix*vec3(0.0f, 0.0f, 1.0f)), 0.0f);
    gl_Position = projection * origin;
    gs_out.FragColor = vec3(0.0f, 0.0f, 1.0f);
    EmitVertex();
    gl_Position = projection * (origin + size * zAxis);
    gs_out.FragColor = vec3(0.0f, 0.0f, 1.0f);
    EmitVertex();

    EndPrimitive();
}

void main()
{
    mat3 nMat = mat3(transpose(inverse(view)));
    CreateAxes(nMat);
}