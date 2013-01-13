
attribute vec3 position;
attribute vec4 color0;
attribute vec2 texcoord0;

uniform mat4 matProj;
uniform mat4 matModel;

varying vec4 color;
varying vec2 texcoord;

void main()
{
	color = color0;
	texcoord = texcoord0;
	
	gl_Position = matProj * matModel * vec4(position, 1.0);
}
