//
//  rn_Texture1.vsh
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

#if !defined(RN_BILLBOARD)
	in vec3 attPosition;
#else //RN_BILLBOARD
	in vec2 attPosition;
#endif
in vec3 attNormal;
#if defined(RN_TEXTURE_DIFFUSE)
in vec2 attTexcoord0;
#endif
in vec4 attTangent;

#if defined(RN_TEXTURE_DIFFUSE)
out vec2 vertTexcoord;
#endif

#if defined(RN_LIGHTING)
	out vec3 vertPosition;
	out vec3 vertNormal;
	#if defined(RN_NORMALMAP)
		out vec3 vertBitangent;
		out vec3 vertTangent;
	#endif
#else
	#if defined(RN_FOG) || defined(RN_CLIPPLANE)
		out vec3 vertPosition;
	#endif
#endif

#if defined(RN_VEGETATION)
	uniform float time;
#endif

void main()
{
	#if defined(RN_TEXTURE_DIFFUSE)
	#if defined(RN_TEXTURE_TILING)
		vertTexcoord = attTexcoord0*RN_TEXTURE_TILING;
	#else
		vertTexcoord = attTexcoord0;
	#endif
	#endif

	#if !defined(RN_BILLBOARD)
		vec4 position = rn_Animate(vec4(attPosition, 1.0));
		vec4 normal   = rn_Animate(vec4(attNormal, 0.0));
		normal.w = 0.0;
		position.w = 1.0;
		#if defined(RN_NORMALMAP)
			vec4 tangent = rn_Animate(vec4(attTangent.xyz, 0.0));
			tangent.w = 0.0;
		#endif
	#else //RN_BILLBOARD
		vec4 normal = vec4(0.0, 0.0, 1.0, 0.0);
		vec4 position = vec4(attPosition, 0.0, 1.0);
		#if defined(RN_NORMALMAP)
			vec4 tangent = vec4(1.0, 0.0, 0.0, 0.0);
		#endif
	#endif
	
	#if defined(RN_VEGETATION)
		position.x += sin(time)*position.y*0.02;
		position.z += cos(time)*position.y*0.02;
	#endif
	
	#if defined(RN_LIGHTING)
		vertPosition = (matModel * position).xyz;
		vertNormal = (matModel * normal).xyz;
		
		#if defined(RN_NORMALMAP)
			vertTangent = (matModel*tangent).xyz;
			vertBitangent = cross(vertNormal, vertTangent)*attTangent.w;
		#endif
	#else
		#if defined(RN_FOG) || defined(RN_CLIPPLANE)
			vertPosition = (matModel * position).xyz;
		#endif
	#endif
	
	#if defined(RN_LIGHTING)
		#if defined(RN_DIRECTIONAL_SHADOWS)
			rn_ShadowDirectional0(position);
		#endif
		#if defined(RN_SPOT_SHADOWS)
			rn_ShadowSpot(position);
		#endif
	#endif
	
	gl_Position = matProjViewModel * position;
}
