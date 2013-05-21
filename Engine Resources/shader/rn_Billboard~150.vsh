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

in vec2 attPosition;
in vec2 attTexcoord0;
in float attNormal;

out vec2 vertTexcoord;

#ifdef RN_LIGHTING
out vec3 vertNormal;
out vec3 vertPosition;
#endif

void main()
{
	vertTexcoord = attTexcoord0;
	
	vec4 normal = vec4(1.0, 0.0, 0.0, 0.0);
	vec4 position = vec4(attPosition, 0.0, 1.0);
	
#ifdef RN_LIGHTING
	vertNormal = (matModel * normal).xyz;
	vertPosition = (matModel * position).xyz;
#endif
	
#if defined(RN_DIRECTIONAL_SHADOWS) && defined(RN_LIGHTING)
	rn_ShadowDir1(position);
#endif

	gl_Position = matProjViewModel * position;
}
