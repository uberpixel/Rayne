#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModel;

in vec2 position;
in vec2 texcoord0;

out vec2 texcoord;

void main()
{
	texcoord = texcoord0;
	gl_Position = matProj * matModel * vec4(position, 1.0, 1.0);
}

