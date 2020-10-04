//
//  Base.hlsl
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#ifndef RN_NORMALS
#define RN_NORMALS 0
#endif

#ifndef RN_ANIMATIONS
#define RN_ANIMATIONS 0
#endif

#ifndef RN_ALPHA
#define RN_ALPHA 0
#endif

#ifndef RN_SKY
#define RN_SKY 0
#endif

#ifndef RN_LIGHTS_DIRECTIONAL
#define RN_LIGHTS_DIRECTIONAL 0
#endif

#ifndef RN_LIGHTS_POINT
#define RN_LIGHTS_POINT 0
#endif

#ifndef RN_SHADOWS_DIRECTIONAL
#define RN_SHADOWS_DIRECTIONAL 0
#endif

#ifndef RN_MAX_BONES
#define RN_MAX_BONES 100
#endif

#ifndef RN_MAX_POINTLIGHTS
#define RN_MAX_POINTLIGHTS 1
#endif

#if RN_ANIMATIONS
#define RN_ANIMATION_VERTEX_UNIFORMS matrix boneMatrices[RN_MAX_BONES]
#define RN_ANIMATION_VERTEX_DATA  \
	[[vk::location(7)]] float4 boneWeights : BONEWEIGHTS; \
	[[vk::location(8)]] float4 boneIndices : BONEINDICES
#define RN_ANIMATION_TRANSFORM(position, vertex) \
	float4( \
		mul(boneMatrices[int(vertex.boneIndices.x)], position).xyz * vertex.boneWeights.x + \
		mul(boneMatrices[int(vertex.boneIndices.y)], position).xyz * vertex.boneWeights.y + \
		mul(boneMatrices[int(vertex.boneIndices.z)], position).xyz * vertex.boneWeights.z + \
		mul(boneMatrices[int(vertex.boneIndices.w)], position).xyz * vertex.boneWeights.w, position.w)
#else
#define RN_ANIMATION_VERTEX_UNIFORMS
#define RN_ANIMATION_VERTEX_DATA
#define RN_ANIMATION_TRANSFORM(position, vertex) position
#endif

struct LightDirectional
{
	float4 direction;
	float4 color;
};

struct PointLight
{
	float4 positionAndRange;
	float4 color;
};





