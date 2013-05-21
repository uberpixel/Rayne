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
in vec3 vertNormal;
#ifdef RN_NORMALMAP
in vec3 vertTangent;
in vec3 vertBitangent;
#endif
#endif

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, vertTexcoord);
	rn_Discard(color0);

#ifdef RN_LIGHTING
	#ifdef RN_NORMALMAP
	vec4 normalspec = texture(mTexture1, vertTexcoord);
	vec3 normal = normalspec.xyz*2.0-1.0;
	mat3 matTangentInv;
	matTangentInv[0] = normalize(vertTangent);
	matTangentInv[1] = normalize(vertBitangent);
	matTangentInv[2] = normalize(vertNormal);
	normal = normalize(matTangentInv*normal);
	
	fragColor0 = rn_Lighting(color0, vec3(normalspec.a), normal, vertPosition);
	#else
	fragColor0 = rn_Lighting(color0, vec3(1.0), normalize(vertNormal), vertPosition);
	#endif
#else
	fragColor0 = color0;
#endif
}
