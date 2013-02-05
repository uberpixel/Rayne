
#version 150
precision highp float;

uniform mat4 matViewModel;
uniform mat4 matProjViewModel;

in vec3 vertPostion;
in vec3 vertNormal;

out vec3 surfaceNormal;
out float surfaceDepth;

float nearPlane = 0.1;
float farPlane = 1000.0;

void main()
{
	surfaceNormal = normalize((matProjViewModel * vec4(vertNormal, 0.0f)).xyz) * vec3(0.5) + vec3(0.5);

	surfaceDepth = (matViewModel * vec4(vertPostion, 1.0)).z;
	surfaceDepth = (-surfaceDepth - nearPlane) / (farPlane - nearPlane);
	
	gl_Position = matProjViewModel * vec4(vertPostion, 1.0);
}
