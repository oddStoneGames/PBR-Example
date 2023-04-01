#version 420 core

in vec3 TexCoord;
out vec4 FragmentColor;

uniform samplerCube skybox;

void main()
{
    vec3 envColor = texture(skybox, TexCoord).rgb;

    // Gamma Correction
    envColor = pow(envColor, vec3(1.0/2.2));

    FragmentColor = vec4(envColor, 1.0);
}