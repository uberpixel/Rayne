#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModel;

in vec3 position;
in vec4 color0;
in vec2 texcoord0;

out vec4 color;
out vec2 texcoord;

void main()
{
	color = color0;
	texcoord = texcoord0;
	
	gl_Position = matProj * matModel * vec4(position, 1.0);
}

