//
//  rn_Terrain.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Lighting.fsh"

uniform sampler2D mTexture0;

in vec2 outTexcoord;
out vec4 fragColor0;

void main()
{
	vec2 texcoord = outTexcoord * vec2(5.0);
	vec4 color0 = texture(mTexture0, texcoord);

	fragColor0 = color0 * rn_Lighting();
}
