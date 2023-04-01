#version 420 core
in vec3 FragColor;

out vec4 FragmentColor;


void main()
{
	FragmentColor = vec4(FragColor, 1.0f);
}