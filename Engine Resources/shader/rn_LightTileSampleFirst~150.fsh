
#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform vec4 frameSize;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec4 depth1 = texture(mTexture0, texcoord-0.25*frameSize.xy);
	vec4 depth2 = texture(mTexture0, texcoord+0.25*frameSize.xy);
	vec4 depth3 = texture(mTexture0, texcoord-0.25*frameSize.xy*vec2(-1.0, 1.0));
	vec4 depth4 = texture(mTexture0, texcoord+0.25*frameSize.xy*vec2(-1.0, 1.0));
	
	fragColor0.r = min(depth1.r, min(depth2.r, min(depth3.r, depth4.r)));
	fragColor0.g = max(depth1.r, max(depth2.r, max(depth3.r, depth4.r)));
	
	float n = 0.1;
	float f = 500.0;
	fragColor0.r = (2.0 * n) / (f + n - fragColor0.r * (f - n));
	fragColor0.g = (2.0 * n) / (f + n - fragColor0.g * (f - n));
	
//	fragColor0.rg *= f-n;
//	fragColor0.g += n;
}
