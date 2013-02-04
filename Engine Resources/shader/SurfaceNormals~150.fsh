#version 150
precision highp float;

in vec3 surfaceNormal;
in float surfaceDepth;

out vec4 fragColor0;

void main()
{
	fragColor0 = vec4(surfaceNormal, surfaceDepth);
	//fragColor0 = vec4(surfaceDepth, surfaceDepth, surfaceDepth, 1.0);
}
