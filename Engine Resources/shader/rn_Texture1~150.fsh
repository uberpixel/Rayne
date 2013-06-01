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

#if defined(RN_NORMALMAP)
	uniform sampler2D mTexture1;
	#if defined(RN_SPECMAP) && defined(RN_SPECULARITY)
		uniform sampler2D mTexture2;
	#endif
#else
	#if defined(RN_SPECMAP) && defined(RN_SPECULARITY)
		uniform sampler2D mTexture1;
	#endif
#endif

#if defined(RN_SPECULARITY)
	uniform vec4 specular;
#endif

#if defined(RN_FOG)
	uniform vec2 fogPlanes;
	uniform vec4 fogColor;
#endif

#if defined(RN_CLIPPLANE)
	uniform vec4 clipPlane;
#endif

in vec2 vertTexcoord;

#if defined(RN_LIGHTING)
	in vec3 vertPosition;
	in vec3 vertNormal;
	#if defined(RN_NORMALMAP)
		in vec3 vertTangent;
		in vec3 vertBitangent;
	#endif
#else
	#if defined(RN_FOG) || defined(RN_CLIPPLANE)
		in vec3 vertPosition;
	#endif
#endif

out vec4 fragColor0;

void main()
{
	#if defined(RN_CLIPPLANE)
		if(dot(clipPlane.xyz, vertPosition)-clipPlane.w < 0.0)
			discard;
	#endif
	
	vec4 color0 = texture(mTexture0, vertTexcoord);
	rn_Discard(color0);

	#if defined(RN_LIGHTING)
		#if defined(RN_NORMALMAP)
			vec4 normalspec = texture(mTexture1, vertTexcoord);
			vec3 normal = normalspec.xyz*2.0-1.0;
			mat3 matTangentInv;
			matTangentInv[0] = normalize(vertTangent);
			matTangentInv[1] = normalize(vertBitangent);
			matTangentInv[2] = normalize(vertNormal);
			normal = normalize(matTangentInv*normal);
			
			#if defined(RN_SPECMAP) && defined(RN_SPECULARITY)
				vec4 spec;
				spec.rgb = specular.rgb*texture(mTexture2, vertTexcoord).rgb;
				spec.a = specular.a;
			#elif defined(RN_SPECULARITY)
				vec4 spec;
				spec.rgb = specular.rgb*normalspec.a;
				spec.a = specular.a;
			#else
				vec4 spec = vec4(0.0);
			#endif
			
			rn_Lighting(color0, spec, normal, vertPosition);
		#else
			#if defined(RN_SPECMAP) && defined(RN_SPECULARITY)
				vec4 spec;
				spec.rgb = specular.rgb*texture(mTexture1, vertTexcoord).rgb;
				spec.a = specular.a;
			#elif defined(RN_SPECULARITY)
				vec4 spec;
				spec.rgb = specular.rgb;
				spec.a = specular.a;
			#else
				vec4 spec = vec4(0.0);
			#endif
			rn_Lighting(color0, spec, normalize(vertNormal), vertPosition);
		#endif
	#endif
	
	#if defined(RN_FOG)
		float camdist = max(min((length(vertPosition-viewPosition)-fogPlanes.x)*fogPlanes.y, 1.0), 0.0);
		color0 = mix(color0, fogColor, camdist);
	#endif
	
	fragColor0 = color0;
}
