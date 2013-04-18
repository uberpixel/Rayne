//
//  rn_Lighting.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_LIGHTING_VSH
#define RN_LIGHTING_VSH

#ifdef RN_LIGHTING

#include "rn_Matrices.vsh"

out vec3 outLightNormal;
out vec3 outLightPosition;

#define rn_Lighting(position, normal) \
	outLightNormal = (matModel * vec4(normal, 0.0)).xyz; \
	outLightPosition = (matModel * vec4(position, 1.0)).xyz;

#else
#define rn_Lighting(position, normal)
#endif

#endif 
