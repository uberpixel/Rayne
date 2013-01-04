
attribute vec3 position;

uniform mat4 matProj;
uniform mat4 matModel;

void main()
{
	gl_Position = matProj * matModel * vec4(position, 1.0);
}
