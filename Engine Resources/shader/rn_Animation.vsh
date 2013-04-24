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
#define RN_MAX_MATBONES 60
#endif

in vec4 vertBoneWeights;
in vec4 vertBoneIndices;

uniform mat4 matBones[RN_MAX_MATBONES];

vec4 rn_Animate(vec4 position)
{
	vec4 pos1 = matBones[int(vertBoneIndices.x)] * position;
	vec4 pos2 = matBones[int(vertBoneIndices.y)] * position;
	vec4 pos3 = matBones[int(vertBoneIndices.z)] * position;
	vec4 pos4 = matBones[int(vertBoneIndices.w)] * position;

	vec4 pos = pos1 * vertBoneWeights.x + pos2 * vertBoneWeights.y + pos3 * vertBoneWeights.z + pos4 * vertBoneWeights.w;
	pos.w = 1.0;

	return pos;
}

#else
#define rn_Animate(position) position
#endif

#endif
