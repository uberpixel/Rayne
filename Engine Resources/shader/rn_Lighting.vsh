//
//  rn_Lighting.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_LIGHTING_VSH
#define RN_LIGHTING_VSH

#include "rn_Shadow.vsh"
#include "rn_Matrices.vsh"

#ifdef RN_LIGHTING

out vec3 outLightNormal;
out vec3 outLightPosition;

void rn_Lighting(vec3 position, vec3 normal)
{
	outLightNormal = (matModel * vec4(normal, 0.0)).xyz;
	outLightPosition = (matModel * vec4(position, 1.0)).xyz;

#ifdef RN_DIRECTIONAL_SHADOWS
	rn_ShadowDir1(vec4(position, 1.0));
#endif
}

#else
#define rn_Lighting(position, normal)
#endif

#endif 
