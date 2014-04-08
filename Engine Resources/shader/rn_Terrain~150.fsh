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
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_NORMAL_WORLD 1
	#define INTERNAL_SAMPLER_DIFFUSE mTexture0
#endif

#if defined(RN_LIGHTING)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_NORMAL_WORLD 1

	#if defined(RN_SPECULARITY)
		#define INTERNAL_NEEDS_SPECULARITY 1

		#if defined(RN_SPECMAP)
			#if !defined(INTERNAL_SAMPLER_DIFFUSE)
				#define INTERNAL_SAMPLER_SPECULAR mTexture0
			#else
				#define INTERNAL_SAMPLER_SPECULAR mTexture1
			#endif
		#endif
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

#if !defined(RN_TEXTURE_TILING)
	#define RN_TEXTURE_TILING 1
#endif


//in out and uniforms

uniform vec4 diffuse;

#if defined(INTERNAL_SAMPLER_DIFFUSE)
	uniform sampler2D INTERNAL_SAMPLER_DIFFUSE;
	uniform sampler2D mTexture1;
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

#if defined(INTERNAL_NEEDS_POSITION_WORLD)
	in vec3 vertPosition;
#endif
#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
	in vec3 vertNormal;
#endif

out vec4 fragColor0;

void main()
{
	#if defined(RN_CLIPPLANE)
		if(dot(clipPlane.xyz, vertPosition)-clipPlane.w < 0.0)
			discard;
	#endif

	#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
		vec3 normal = normalize(vertNormal);
	#endif
	
	#if defined(RN_TEXTURE_DIFFUSE)
		vec3 scaledPosition = vertPosition.xyz * RN_TEXTURE_TILING;
		vec3 blendFactors = abs(normal);
		blendFactors = max((blendFactors - 0.2) * 7.0, 0.0); 
		blendFactors /= blendFactors.x + blendFactors.y + blendFactors.z;

		vec3 texX = texture(mTexture1, scaledPosition.yz).rgb;
		vec3 texY = texture(INTERNAL_SAMPLER_DIFFUSE, scaledPosition.xz).rgb;
		vec3 texZ = texture(mTexture1, scaledPosition.xy).rgb;

		vec4 color0 = vec4(1.0);
		color0.rgb = texX * blendFactors.x + texY * blendFactors.y + texZ * blendFactors.z;
		color0 *= diffuse;
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

		rn_Lighting(color0, spec, normal, vertPosition);
	#endif
	
	#if defined(RN_FOG)
		float camdist = max(min((length(vertPosition-viewPosition)-fogPlanes.x)*fogPlanes.y, 1.0), 0.0);
		color0 = mix(color0, fogColor, camdist);
	#endif
	
	fragColor0 = color0;
}
