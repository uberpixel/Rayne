#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModel;

in vec3 position;
in vec4 color0;

out vec4 color;

void main()
{
	color = color0;
	gl_Position = matProj * matModel * vec4(position, 1.0);
}

