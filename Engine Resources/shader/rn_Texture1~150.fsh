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

#ifdef RN_LIGHTING
in vec3 outNormal;
in vec3 outPosition;
#endif

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, outTexcoord);
	rn_Discard(color0);

#ifdef RN_LIGHTING
	fragColor0 = rn_Lighting(color0, normalize(outNormal), outPosition);
#else
	fragColor0 = color0;
#endif
}
