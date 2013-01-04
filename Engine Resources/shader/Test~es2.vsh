
attribute vec3 position;
attribute vec4 color0;

uniform mat4 matProj;
uniform mat4 matModel;

varying vec4 color;

void main()
{
	color = color0;
	gl_Position = matProj * matModel * vec4(position, 1.0);
}
