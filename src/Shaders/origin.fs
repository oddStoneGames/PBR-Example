#version 420 core
in GS_OUT
{
    vec3 FragColor;
}fs_in;

out vec4 FragmentColor;

void main()
{
    gl_FragDepth = 0.0f;
    FragmentColor = vec4(fs_in.FragColor, 1.0f);
}