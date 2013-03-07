//
//  rn_Animation.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#define RN_ANIMATION_VSH

#ifdef RN_ANIMATION

#ifndef RN_MAX_MATBONES
#define RN_MAX_MATBONES 60
#endif

in vec4 vertBoneWeights;
in vec4 vertBoneIndices;

uniform mat4 matBones[RN_MAX_MATBONES];

vec4 rn_Animate(vec3 position)
{
	vec4 pos = vec4(position, 1.0);

	vec4 pos1 = matBones[int(vertBoneIndices.x)] * pos;
	vec4 pos2 = matBones[int(vertBoneIndices.y)] * pos;
	vec4 pos3 = matBones[int(vertBoneIndices.z)] * pos;
	vec4 pos4 = matBones[int(vertBoneIndices.w)] * pos;

	pos = pos1 * vertBoneWeights.x + pos2 * vertBoneWeights.y + pos3 * vertBoneWeights.z + pos4 * vertBoneWeights.w;
	pos.w = 1.0;

	return pos;
}

#else

#define rn_Animate(position) vec4(position, 1.0)

#endif
