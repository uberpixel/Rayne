#version 150
precision mediump float;
 
uniform sampler2D targetmap0;
 
in vec2 texcoord;
in vec2 blurTexcoords[14];

out vec4 fragColor0;
 
void main()
{
	fragColor0 = vec4(0.0);
	fragColor0 += texture(targetmap0, blurTexcoords[ 0]) * 0.0044299121055113265;
	fragColor0 += texture(targetmap0, blurTexcoords[ 1]) * 0.00895781211794;
	fragColor0 += texture(targetmap0, blurTexcoords[ 2]) * 0.0215963866053;
	fragColor0 += texture(targetmap0, blurTexcoords[ 3]) * 0.0443683338718;
	fragColor0 += texture(targetmap0, blurTexcoords[ 4]) * 0.0776744219933;
	fragColor0 += texture(targetmap0, blurTexcoords[ 5]) * 0.115876621105;
	fragColor0 += texture(targetmap0, blurTexcoords[ 6]) * 0.147308056121;
	fragColor0 += texture(targetmap0, texcoord         ) * 0.159576912161;
	fragColor0 += texture(targetmap0, blurTexcoords[ 7]) * 0.147308056121;
	fragColor0 += texture(targetmap0, blurTexcoords[ 8]) * 0.115876621105;
	fragColor0 += texture(targetmap0, blurTexcoords[ 9]) * 0.0776744219933;
	fragColor0 += texture(targetmap0, blurTexcoords[10]) * 0.0443683338718;
	fragColor0 += texture(targetmap0, blurTexcoords[11]) * 0.0215963866053;
	fragColor0 += texture(targetmap0, blurTexcoords[12]) * 0.00895781211794;
	fragColor0 += texture(targetmap0, blurTexcoords[13]) * 0.0044299121055113265;
}