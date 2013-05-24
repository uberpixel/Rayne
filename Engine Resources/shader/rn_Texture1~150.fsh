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
#ifdef RN_SPECMAP && RN_SPECULARITY
uniform sampler2D mTexture2;
#endif
#else
#ifdef RN_SPECMAP && RN_SPECULARITY
uniform sampler2D mTexture1;
#endif
#endif

#ifdef RN_SPECULARITY
uniform vec4 specular;
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
	
	#ifdef RN_SPECMAP && RN_SPECULARITY
	vec4 spec;
	spec.rgb = specular.rgb*texture(mTexture2, vertTexcoord).rgb;
	spec.a = specular.a*128.0;
	#else
	#ifdef RN_SPECULARITY
	vec4 spec;
	spec.rgb = specular.rgb*normalspec.a;
	spec.a = specular.a*128.0;
	#else
	vec4 spec = vec4(0.0);
	#endif
	#endif
	
	rn_Lighting(color0, spec, normal, vertPosition);
	
	#else
	#ifdef RN_SPECMAP && RN_SPECULARITY
	vec4 spec;
	spec.rgb = specular.rgb*texture(mTexture1, vertTexcoord).rgb;
	spec.a = specular.a*128.0;
	#else
	#ifdef RN_SPECULARITY
	vec4 spec;
	spec.rgb = specular.rgb;
	spec.a = specular.a*128.0;
	#else
	vec4 spec = vec4(0.0);
	#endif
	#endif
	rn_Lighting(color0, spec, normalize(vertNormal), vertPosition);
	#endif
#endif
	
	fragColor0 = color0;
}
