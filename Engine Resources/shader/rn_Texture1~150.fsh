//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Lighting.fsh"
#include "rn_Discard.fsh"

uniform sampler2D mTexture0;

in vec2 outTexcoord;
out vec4 fragColor0;

void main()
{
#ifndef PURPLE
	vec4 color0 = texture(mTexture0, outTexcoord);
	rn_Discard(color0);
#else
	vec4 color0 = vec4(0.851, 0.283, 0.744, 1.000);
#endif

	fragColor0 = color0 * rn_Lighting();
}
