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
uniform sampler2D mTexture1;

in vec2 outTexcoord;
out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, outTexcoord);
	vec4 color1 = texture(mTexture1, outTexcoord);

	rn_Discard(color0);

	fragColor0 = (color0 * color1) * rn_Lighting();
}
