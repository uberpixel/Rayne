#version 150
precision mediump float;
 
uniform sampler2D targetmap0;
 
in vec2 texcoord;
in vec2 blurTexcoords[8];

out vec4 fragColor0;
 
void main()
{
	fragColor0 = vec4(0.0);
//	fragColor0 += texture(targetmap0, blurTexcoords[ 0]) * 0.0044299121055113265;
//	fragColor0 += texture(targetmap0, blurTexcoords[ 1]) * 0.00895781211794;
//	fragColor0 += texture(targetmap0, blurTexcoords[ 2]) * 0.0215963866053;
	fragColor0 += texture(targetmap0, blurTexcoords[ 0]) * 1.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 1]) * 8.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 2]) * 28.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 3]) * 56.0/256.0;
	fragColor0 += texture(targetmap0, texcoord         ) * 70.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 4]) * 56.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 5]) * 28.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 6]) * 8.0/256.0;
	fragColor0 += texture(targetmap0, blurTexcoords[ 7]) * 1.0/256.0;
//	fragColor0 += texture(targetmap0, blurTexcoords[11]) * 0.0215963866053;
//	fragColor0 += texture(targetmap0, blurTexcoords[12]) * 0.00895781211794;
//	fragColor0 += texture(targetmap0, blurTexcoords[13]) * 0.0044299121055113265;
}