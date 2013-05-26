//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
in vec2 attTexcoord0;
in vec4 attTangent;

out vec2 vertTexcoord;

#if defined(RN_LIGHTING)
	out vec3 vertPosition;
	out vec3 vertNormal;
	#if defined(RN_NORMALMAP)
		out vec3 vertBitangent;
		out vec3 vertTangent;
	#endif
#endif

void main()
{
	#if defined(RN_TEXTURE_TILING)
		vertTexcoord = attTexcoord0*RN_TEXTURE_TILING;
	#else
		vertTexcoord = attTexcoord0;
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
		vec4 normal = vec4(0.0, 0.0, -1.0, 0.0);
		vec4 position = vec4(attPosition, 0.0, 1.0);
		#if defined(RN_NORMALMAP)
			vec4 tangent = vec4(1.0, 0.0, 0.0, 0.0);
		#endif
	#endif
	
	#if defined(RN_LIGHTING)
		vertPosition = (matModel * position).xyz;
		vertNormal = (matModel * normal).xyz;
		
		#if defined(RN_NORMALMAP)
			vertTangent = (matModel*tangent).xyz;
			vertBitangent = cross(vertNormal, vertTangent)*attTangent.w;
		#endif
	#endif
	
	#if defined(RN_DIRECTIONAL_SHADOWS) && defined(RN_LIGHTING)
		rn_ShadowDir1(position);
	#endif
	
	gl_Position = matProjViewModel * position;
}
