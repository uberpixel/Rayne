//
//  rn_LightTileSampleFirst.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform vec4 frameSize;
uniform vec2 clipPlanes;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec4 depth1 = texture(mTexture0, texcoord-0.5*frameSize.xy);
	vec4 depth2 = texture(mTexture0, texcoord+0.5*frameSize.xy);
	vec4 depth3 = texture(mTexture0, texcoord-0.5*frameSize.xy*vec2(-1.0, 1.0));
	vec4 depth4 = texture(mTexture0, texcoord+0.5*frameSize.xy*vec2(-1.0, 1.0));
	
	fragColor0.r = min(depth1.r, min(depth2.r, min(depth3.r, depth4.r)));
	fragColor0.g = max(depth1.r, max(depth2.r, max(depth3.r, depth4.r)));
	
	fragColor0.rg = (clipPlanes.x * clipPlanes.y)/(clipPlanes.y-fragColor0.rg*(clipPlanes.y-clipPlanes.x));
}
