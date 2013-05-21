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

in vec2 vertTexcoord;

#ifdef RN_LIGHTING
in vec3 vertNormal;
in vec3 vertPosition;
#endif

out vec4 fragColor0;

void main()
{
	vec2 texcoord = vertTexcoord * vec2(8.0);
	vec4 color0 = texture(mTexture0, texcoord);

#ifdef RN_LIGHTING
	fragColor0 = rn_Lighting(color0, normalize(vertNormal), vertPosition);
#else
	fragColor0 = color0;
#endif
}
