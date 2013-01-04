
attribute vec2 position;
attribute vec2 texcoord0;

uniform mat4 matProj;

varying vec2 texcoord;

void main()
{
	texcoord = texcoord0;
	gl_Position = matProj * vec4(position, 1.0, 1.0);
}
