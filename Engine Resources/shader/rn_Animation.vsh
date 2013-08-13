//
//  rn_Animation.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_ANIMATION_VSH
#define RN_ANIMATION_VSH

#ifdef RN_ANIMATION

#ifndef RN_MAX_MATBONES
#define RN_MAX_MATBONES 100
#endif

in vec4 attBoneWeights;
in vec4 attBoneIndices;

uniform mat4 matBones[RN_MAX_MATBONES];

vec4 rn_Animate(vec4 position)
{
	vec4 pos1 = matBones[int(attBoneIndices.x)] * position;
	vec4 pos2 = matBones[int(attBoneIndices.y)] * position;
	vec4 pos3 = matBones[int(attBoneIndices.z)] * position;
	vec4 pos4 = matBones[int(attBoneIndices.w)] * position;

	vec4 pos = pos1 * attBoneWeights.x + pos2 * attBoneWeights.y + pos3 * attBoneWeights.z + pos4 * attBoneWeights.w;
	pos.w = 1.0;

	return pos;
}

#else
#define rn_Animate(position) position
#endif

#endif
