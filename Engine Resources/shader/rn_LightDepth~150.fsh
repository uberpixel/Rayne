//
//  rn_Color1.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Discard.fsh"

uniform sampler2D mTexture0;

in vec2 outTexcoord;
in vec3 outStuff;
out vec4 fragColor0;

void main()
{
#ifdef RN_DISCARD
	vec4 color0 = texture(mTexture0, outTexcoord);
	rn_Discard(color0);
#endif

	fragColor0 = vec4(vec2(outStuff.xy/outStuff.z*0.5+0.5), 0.0, 1.0);
//	fragColor0 = vec4(vec3(gl_FragCoord.z), 1.0);
}
