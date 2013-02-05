#version 150
precision highp float;

in vec2 vertPosition;
in vec2 vertTexcoord0;

out vec2 texcoord;
out vec2 blurTexcoords[14];

void main()
{
	texcoord = vertTexcoord0;

	blurTexcoords[ 0] = texcoord + vec2(-0.028, 0.0);
	blurTexcoords[ 1] = texcoord + vec2(-0.024, 0.0);
	blurTexcoords[ 2] = texcoord + vec2(-0.020, 0.0);
	blurTexcoords[ 3] = texcoord + vec2(-0.016, 0.0);
	blurTexcoords[ 4] = texcoord + vec2(-0.012, 0.0);
	blurTexcoords[ 5] = texcoord + vec2(-0.008, 0.0);
	blurTexcoords[ 6] = texcoord + vec2(-0.004, 0.0);
	blurTexcoords[ 7] = texcoord + vec2( 0.004, 0.0);
	blurTexcoords[ 8] = texcoord + vec2( 0.008, 0.0);
	blurTexcoords[ 9] = texcoord + vec2( 0.012, 0.0);
	blurTexcoords[10] = texcoord + vec2( 0.016, 0.0);
	blurTexcoords[11] = texcoord + vec2( 0.020, 0.0);
	blurTexcoords[12] = texcoord + vec2( 0.024, 0.0);
	blurTexcoords[13] = texcoord + vec2( 0.028, 0.0);

	gl_Position = vec4(vertPosition, 0.0, 1.0);
}
