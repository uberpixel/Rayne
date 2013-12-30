//
//  rn_Color1.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Discard.fsh"

uniform sampler2D mTexture0;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
#ifdef RN_DISCARD
	vec4 color0 = texture(mTexture0, vertTexcoord);
	rn_Discard(color0);
#endif

	fragColor0 = vec4(vec3(gl_FragCoord.z), 1.0);
}
