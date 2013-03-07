//
//  rn_Lightning.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#define RN_LIGHTNING_VSH

#ifdef RN_LIGHTNING

out vec3 outLightNormal;
out vec3 outLightPosition;

vec3 rn_Lightning(mat4 model, vec3 position, vec3 normal)
{
#ifdef RN_ANIMATION_VSH
	vec4 pos = rn_Animate(vec4(position, 1.0));
	vec4 norm = rn_Animate(vec4(normal, 0.0));
	norm.w = 0.0;
	 
	outLightNormal = (model * norm).xyz;
	outLightPosition = (model * pos).xyz;

	return pos.xyz;
#else
	outLightNormal = (model * vec4(normal, 0.0)).xyz;
	outLightPosition = (model * vec4(position, 1.0)).xyz;

	return position;
#endif
}

#else

#ifdef RN_ANIMATION_VSH

vec3 rn_Lightning(mat4 model, vec3 position, vec3 normal)
{
	vec4 pos = rn_Animate(vec4(position, 1.0));
	return pos.xyz;
}

#else
#define rn_Lightning(model, position, normal) (position)
#endif

#endif
