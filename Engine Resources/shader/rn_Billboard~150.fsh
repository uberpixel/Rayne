//
//  rn_Billboard.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Discard.fsh"
#include "rn_Lighting.fsh"

uniform sampler2D mTexture0;

in vec2 vertTexcoord;

#ifdef RN_LIGHTING
in vec3 vertNormal;
in vec3 vertPosition;
#endif

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, vertTexcoord);
	rn_Discard(color0);

#ifdef RN_LIGHTING
	rn_Lighting(color0, vec3(1.0), normalize(vertNormal), vertPosition);
#endif
	fragColor0 = color0;
}
