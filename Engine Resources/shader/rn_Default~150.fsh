//
//  rn_Default.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Lighting.fsh"
#include "rn_Discard.fsh"


// internal define setup

#if defined(RN_TEXTURE_DIFFUSE)
	#define INTERNAL_NEEDS_UV0 1
	#define INTERNAL_SAMPLER_DIFFUSE mTexture0
#endif

#if defined(RN_LIGHTING)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_NORMAL_WORLD 1

	#if defined(RN_NORMALMAP)
		#define INTERNAL_NEEDS_TANGENT_WORLD 1
		#define INTERNAL_NEEDS_BITANGENT_WORLD 1
		#define INTERNAL_NEEDS_UV0 1

		#if !defined(INTERNAL_SAMPLER_DIFFUSE)
			#define INTERNAL_SAMPLER_NORMAL mTexture0
		#else
			#define INTERNAL_SAMPLER_NORMAL mTexture1
		#endif
	#endif

	#if defined(RN_SPECULARITY)
		#define INTERNAL_NEEDS_SPECULARITY 1

		#if defined(RN_SPECMAP)
			#if !defined(INTERNAL_SAMPLER_DIFFUSE) && !defined(INTERNAL_SAMPLER_NORMAL)
				#define INTERNAL_SAMPLER_SPECULAR mTexture0
			#elif (defined(INTERNAL_SAMPLER_DIFFUSE) && !defined(INTERNAL_SAMPLER_NORMAL)) || (!defined(INTERNAL_SAMPLER_DIFFUSE) && defined(INTERNAL_SAMPLER_NORMAL))
				#define INTERNAL_SAMPLER_SPECULAR mTexture1
			#else
				#define INTERNAL_SAMPLER_SPECULAR mTexture2
			#endif
		#endif
	#endif
#endif

#if defined(RN_GLASS)
	#define INTERNAL_NEEDS_POSITION_WORLD 1

	#if !defined(RN_LIGHTING)
		uniform vec3 viewPosition;
	#endif
#endif

#if defined(RN_FOG)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_FOG 1
#endif
#if defined(RN_CLIPPLANE)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_CLIPPLANE 1
#endif


//in out and uniforms

uniform vec4 diffuse;

#if defined(INTERNAL_SAMPLER_DIFFUSE)
	uniform sampler2D INTERNAL_SAMPLER_DIFFUSE;
#endif

#if defined(INTERNAL_SAMPLER_NORMAL)
	uniform sampler2D INTERNAL_SAMPLER_NORMAL;
#endif

#if defined(INTERNAL_SAMPLER_SPECULAR)
	uniform sampler2D INTERNAL_SAMPLER_SPECULAR;
#endif

#if defined(INTERNAL_NEEDS_SPECULARITY)
	uniform vec4 specular;
#endif

#if defined(INTERNAL_NEEDS_FOG)
	uniform vec2 fogPlanes;
	uniform vec4 fogColor;
#endif

#if defined(INTERNAL_NEEDS_CLIPPLANE)
	uniform vec4 clipPlane;
#endif

#if defined(INTERNAL_NEEDS_UV0)
	in vec2 vertTexcoord;
#endif

#if defined(INTERNAL_NEEDS_POSITION_WORLD)
	in vec3 vertPosition;
#endif
#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
	in vec3 vertNormal;
#endif
#if defined(INTERNAL_NEEDS_TANGENT_WORLD)
	in vec3 vertTangent;
#endif
#if defined(INTERNAL_NEEDS_BITANGENT_WORLD)
		in vec3 vertBitangent;
#endif

out vec4 fragColor0;

void main()
{
	#if defined(RN_CLIPPLANE)
		if(dot(clipPlane.xyz, vertPosition)-clipPlane.w < 0.0)
			discard;
	#endif
	
	#if defined(RN_TEXTURE_DIFFUSE)
		vec4 color0 = texture(INTERNAL_SAMPLER_DIFFUSE, vertTexcoord) * diffuse;
	#else
		vec4 color0 = diffuse;
	#endif

	rn_Discard(color0);

	#if defined(RN_LIGHTING)
		vec4 spec = vec4(0.0f);
		#if defined(RN_SPECULARITY)
			spec = specular;
			#if defined(RN_SPECMAP)
				spec.rgb *= texture(INTERNAL_SAMPLER_SPECULAR, vertTexcoord).rgb;
			#endif
		#endif

		#if defined(RN_NORMALMAP)
			vec4 normalspec = texture(INTERNAL_SAMPLER_NORMAL, vertTexcoord);
			vec3 normal = normalspec.xyz*2.0-1.0;
			mat3 matTangentInv;
			matTangentInv[0] = normalize(vertTangent);
			matTangentInv[1] = normalize(vertBitangent);
			matTangentInv[2] = normalize(vertNormal);
			normal = normalize(matTangentInv*normal);

			#if defined(RN_SPECULARITY) && !defined(RN_SPECMAP)
				spec.rgb *= normalspec.a;
			#endif
		#else
			vec3 normal = normalize(vertNormal);
		#endif
		
		rn_Lighting(color0, spec, normal, vertPosition);
	#endif

	#if defined(RN_GLASS)
		color0.a = max(dot(normalize(vertNormal), normalize(viewPosition-vertPosition)), 0.0)*2.0;
		color0.a *= color0.a;
		color0.a *= color0.a;
		color0.a = clamp(0.3-color0.a, 0.0, 1.0);
		color0.rgb = mix(color0.rgb, vec3(1.0), color0.a*0.04);
	#endif
	
	#if defined(RN_FOG)
		float camdist = max(min((length(vertPosition-viewPosition)-fogPlanes.x)*fogPlanes.y, 1.0), 0.0);
		color0 = mix(color0, fogColor, camdist);
	#endif
	
	fragColor0 = color0;
}
