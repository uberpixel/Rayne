#version 150
precision highp float;

in vec3 surfaceNormal;
in float surfaceDepth;

out vec4 fragColor0;

void main()
{
	fragColor0 = vec4(normalize(surfaceNormal), surfaceDepth);
}
