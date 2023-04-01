#version 420 core
layout(triangles) in;
layout(line_strip, max_vertices = 8) out;

in vec3 Normal[];

out vec3 FragColor;

const float MAGNITUDE = 0.05f;
uniform uint showFaceNormal;
uniform uint showVertexNormal;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void ShowNormal(mat4 viewMeshModel, int index)
{
    mat3 normalMatrix = mat3(transpose(inverse(viewMeshModel)));

    //Emit Position Vertex.
    vec4 vertexPos = viewMeshModel * gl_in[index].gl_Position;
    gl_Position = projection * vertexPos;
    FragColor = vec3(0.0f, 1.0f, 0.0f);
    EmitVertex();

    //Emit Normal Vertex.
    gl_Position = projection * (vertexPos + vec4(MAGNITUDE * normalize(normalMatrix*Normal[index]), 0.0f));
    FragColor = vec3(0.0f, 1.0f, 0.0f);
    EmitVertex();

    EndPrimitive();
}

void ShowFaceNormal(mat4 viewMeshModel)
{
    mat3 normalMatrix = mat3(transpose(inverse(viewMeshModel)));

    //Get Centroid Of Triangle.
    vec4 centroidOfTriangle = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3.0f;

    //Emit Position Vertex.
    vec4 vertexPos = viewMeshModel * centroidOfTriangle;
    gl_Position = projection * vertexPos;
    FragColor = vec3(0.0f, 0.0f, 1.0f);
    EmitVertex();

    //Get Weigthed Normal.
    vec4 normal = vec4(MAGNITUDE * normalize((normalMatrix*Normal[0] + normalMatrix*Normal[1] + normalMatrix*Normal[2]) / 3.0f), 0.0f);

    //Emit Normal Vertex.
    gl_Position = projection * (vertexPos + normal);
    FragColor = vec3(0.0f, 0.0f, 1.0f);
    EmitVertex();

    EndPrimitive();
}

void main()
{
    mat4 viewMeshModelMatrix = view * model;
    if(showVertexNormal > 0)
    {
        ShowNormal (viewMeshModelMatrix, 0);
        ShowNormal (viewMeshModelMatrix, 1);
        ShowNormal (viewMeshModelMatrix, 2);
    }
    if(showFaceNormal > 0)
        ShowFaceNormal(viewMeshModelMatrix);
}