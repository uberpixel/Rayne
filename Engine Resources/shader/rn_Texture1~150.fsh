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

#ifdef RN_NORMALMAP
uniform sampler2D mTexture1;
#endif

in vec2 vertTexcoord;

#ifdef RN_LIGHTING
in vec3 vertPosition;
#ifdef RN_NORMALMAP
in mat3 vertMatInvTangent
#else
in vec3 vertNormal;
#endif
#endif

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, vertTexcoord);
	rn_Discard(color0);

#ifdef RN_LIGHTING
	#ifdef RN_NORMALMAP
	vec3 normal = texture(mTexture1, vertTexcoord).xyz*2.0-1.0;
	normal = vertMatInvTangent*normal;
	
	fragColor0 = rn_Lighting(color0, normalize(normal), vertPosition);
	#else
	fragColor0 = rn_Lighting(color0, normalize(vertNormal), vertPosition);
	#endif
#else
	fragColor0 = color0;
#endif
}
