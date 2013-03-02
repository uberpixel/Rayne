//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Lightning.fsh"

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;

in vec2 outTexcoord;
in vec3 outNormal;
in vec3 outPosition;

out vec4 fragColor0;

void main()
{
	fragColor0 = texture(mTexture0, outTexcoord);
	if(fragColor0.a < 0.3)
		discard;

	fragColor0.rgb *= rn_calculateLight(outNormal, outPosition);
}
