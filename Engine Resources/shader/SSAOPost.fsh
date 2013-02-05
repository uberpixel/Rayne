
#version 150
precision highp float;

uniform sampler2D mTexture0; // SSAO
uniform sampler2D targetmap0; // Scene

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec4 scene = texture(targetmap0, texcoord);
	float ssao = texture(mTexture0, texcoord).a;

	fragColor0 = vec4(ssao, ssao, ssao, 1.0);

	//fragColor0 = scene * ssao;
}
