#version 150
precision highp float;

in vec2 vertPosition;
in vec2 vertTexcoord0;

out vec2 texcoord;

void main()
{
	texcoord = vertTexcoord0;
	gl_Position = vec4(vertPosition, 0.0, 1.0);
}