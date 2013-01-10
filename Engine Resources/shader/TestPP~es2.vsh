
uniform mat4 matProj;
uniform mat4 matModel;

attribute vec2 position;
attribute vec2 texcoord0;

varying vec2 texcoord;

void main()
{
	texcoord = texcoord0;
	gl_Position = matProj * matModel * vec4(position, 1.0, 1.0);
}

