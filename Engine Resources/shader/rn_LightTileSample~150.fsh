//
//  rn_LightTileSample.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform vec4 frameSize;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec4 depth1 = texture(mTexture0, texcoord-0.5*frameSize.xy);
	vec4 depth2 = texture(mTexture0, texcoord+0.5*frameSize.xy);
	vec4 depth3 = texture(mTexture0, texcoord-0.5*frameSize.xy*vec2(-1.0, 1.0));
	vec4 depth4 = texture(mTexture0, texcoord+0.5*frameSize.xy*vec2(-1.0, 1.0));
	
	fragColor0.r = max(depth1.r, max(depth2.r, max(depth3.r, depth4.r)));
	fragColor0.g = min(depth1.g, min(depth2.g, min(depth3.g, depth4.g)));
}
