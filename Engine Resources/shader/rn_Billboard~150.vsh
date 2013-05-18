//
//  rn_Billboard.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Shadow.vsh"

in vec2 vertPosition;
in vec2 vertTexcoord0;
in float vertNormal;

out vec2 outTexcoord;

#ifdef RN_LIGHTING
out vec3 outNormal;
out vec3 outPosition;
#endif

void main()
{
	outTexcoord = vertTexcoord0;
	
	vec4 normal = vec4(1.0, 0.0, 0.0, 0.0);
	vec4 position = vec4(vertPosition, 0.0, 1.0);
	
#ifdef RN_LIGHTING
	outNormal = (matModel * normal).xyz;
	outPosition = (matModel * position).xyz;
#endif
	
#if defined(RN_DIRECTIONAL_SHADOWS) && defined(RN_LIGHTING)
	rn_ShadowDir1(position);
#endif

	gl_Position = matProjViewModel * position;
}
