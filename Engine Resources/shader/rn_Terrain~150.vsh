//
//  rn_Default.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"
#include "rn_Shadow.vsh"


// internal define setup

#if defined(RN_TEXTURE_DIFFUSE)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_NORMAL_WORLD 1
#endif

#if defined(RN_LIGHTING)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_NORMAL_WORLD 1

	#if defined(RN_DIRECTIONAL_SHADOWS)
		#define INTERNAL_SHADOWS_DIRECTIONAL 1
	#endif

	#if defined(RN_DIRECTIONAL_SHADOWS)
		#define INTERNAL_SHADOWS_SPOT 1
	#endif
#endif


#if defined(RN_FOG) || defined(RN_CLIPPLANE)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
#endif

#if defined(RN_VEGETATION)
	#define INTERNAL_NEEDS_TIME 1
	#define INTERNAL_ANIMATION_VEGETATION 1
#endif


// in out and uniform variables

in vec3 attPosition;

#if defined(INTERNAL_NEEDS_TANGENT_WORLD) || defined(INTERNAL_NEEDS_BITANGENT_WORLD)
	in vec4 attTangent;
#endif
#if defined(INTERNAL_NEEDS_POSITION_WORLD)
	out vec3 vertPosition;
#endif
#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
	in vec3 attNormal;
	out vec3 vertNormal;
#endif

void main()
{
	vec4 position = vec4(attPosition, 1.0);

	#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
		vec4 normal   = vec4(attNormal, 0.0);
	#endif
	
	#if defined(INTERNAL_NEEDS_POSITION_WORLD)
		vertPosition = (matModel * position).xyz;
	#endif
	#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
		vertNormal = (matModel * normal).xyz;
	#endif
	
	#if defined(INTERNAL_SHADOWS_DIRECTIONAL)
		rn_ShadowDirectional0(vec4(vertPosition, 1.0));
	#endif
	#if defined(INTERNAL_SHADOWS_SPOT)
		rn_ShadowSpot(vec4(vertPosition, 1.0));
	#endif
	
	gl_Position = matProjViewModel * position;
}
