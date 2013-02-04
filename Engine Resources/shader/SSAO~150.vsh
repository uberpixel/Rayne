#version 150
precision highp float;

in vec2 position;
in vec2 texcoord0;

out vec2 texcoord;

void main()
{
	texcoord = texcoord0;
	gl_Position = vec4(position, 0.0, 1.0);
}