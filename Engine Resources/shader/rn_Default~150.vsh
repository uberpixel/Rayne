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

#if defined(RN_BILLBOARD)
	#define INTERNAL_MESH_2D 1
#endif

#if defined(RN_TEXTURE_DIFFUSE)
	#define INTERNAL_NEEDS_UV0 1
#endif

#if defined(RN_LIGHTING)
	#define INTERNAL_NEEDS_POSITION_WORLD 1
	#define INTERNAL_NEEDS_NORMAL_WORLD 1

	#if defined(RN_NORMALMAP)
		#define INTERNAL_NEEDS_TANGENT_WORLD 1
		#define INTERNAL_NEEDS_BITANGENT_WORLD 1
	#endif

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

#if defined(INTERNAL_MESH_2D)
	in vec2 attPosition;
#else //INTERNAL_MESH_2D
	in vec3 attPosition;
#endif

#if defined(INTERNAL_NEEDS_UV0)
	in vec2 attTexcoord0;
	out vec2 vertTexcoord;
#endif

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
#if defined(INTERNAL_NEEDS_BITANGENT_WORLD)
	out vec3 vertBitangent;
#endif
#if defined(INTERNAL_NEEDS_TANGENT_WORLD)
	out vec3 vertTangent;
#endif

#if defined(INTERNAL_NEEDS_TIME)
	uniform float time;
#endif

void main()
{
	#if defined(INTERNAL_NEEDS_UV0)
		#if defined(RN_TEXTURE_TILING)
			vertTexcoord = attTexcoord0*RN_TEXTURE_TILING;
		#else
			vertTexcoord = attTexcoord0;
		#endif
	#endif

	#if !defined(INTERNAL_MESH_2D)
		vec4 position = rn_Animate(vec4(attPosition, 1.0));
		position.w = 1.0;

		#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
			vec4 normal   = rn_Animate(vec4(attNormal, 0.0));
			normal.w = 0.0;
		#endif
		#if defined(INTERNAL_NEEDS_TANGENT_WORLD)
			vec4 tangent = rn_Animate(vec4(attTangent.xyz, 0.0));
			tangent.w = 0.0;
		#endif
	#else //INTERNAL_MESH_2D
		vec4 position = vec4(attPosition, 0.0, 1.0);

		#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
			vec4 normal = vec4(0.0, 0.0, 1.0, 0.0);
		#endif
		#if defined(INTERNAL_NEEDS_TANGENT_WORLD)
			vec4 tangent = vec4(1.0, 0.0, 0.0, 0.0);
		#endif
	#endif
	
	#if defined(INTERNAL_ANIMATION_VEGETATION)
		position.x += sin(time)*position.y*0.02;
		position.z += cos(time)*position.y*0.02;
	#endif
	
	#if defined(INTERNAL_NEEDS_POSITION_WORLD)
		vertPosition = (matModel * position).xyz;
	#endif
	#if defined(INTERNAL_NEEDS_NORMAL_WORLD)
		vertNormal = (matModel * normal).xyz;
	#endif
	#if defined(INTERNAL_NEEDS_TANGENT_WORLD)
		vertTangent = (matModel*tangent).xyz;
	#endif
	#if defined(INTERNAL_NEEDS_BITANGENT_WORLD)
		vertBitangent = cross(vertNormal, vertTangent)*attTangent.w;
	#endif
	
	#if defined(INTERNAL_SHADOWS_DIRECTIONAL)
		rn_ShadowDirectional0(vec4(vertPosition, 1.0));
	#endif
	#if defined(INTERNAL_SHADOWS_SPOT)
		rn_ShadowSpot(vec4(vertPosition, 1.0));
	#endif
	
	gl_Position = matProjViewModel * position;
}
