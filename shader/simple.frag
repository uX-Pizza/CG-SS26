#version 330 core

out vec3 fragColor;
in vec3 fragmentColor;

void main()
{
	fragColor = fragmentColor;
}

