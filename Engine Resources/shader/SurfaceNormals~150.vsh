
#version 150
precision highp float;

uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjViewModel;
uniform mat4 matProjViewModelInverse;

in vec3 position;
in vec3 normal;

out vec3 surfaceNormal;
out float surfaceDepth;

float nearPlane = 0.001;
float farPlane = 100.0;

void main()
{
	mat4 modelView = matModel * matView;

	mat3 foo = mat3(inverse(matProjViewModel));
	surfaceNormal = normalize(vec3(foo * normal)) * vec3(0.5) + vec3(0.5);

	surfaceDepth = (modelView * vec4(position, 1.0)).z;
	surfaceDepth = (-surfaceDepth - nearPlane) / (farPlane - nearPlane);
	
	gl_Position = matProjViewModel * vec4(position, 1.0);
}
