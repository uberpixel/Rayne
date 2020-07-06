//
//  Depth.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Exported defines:
// RN_UV0
// RN_ALPHA
// RN_SKY

#ifndef RN_MAX_BONES
#define RN_MAX_BONES 100
#endif

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_ALPHA
#define RN_ALPHA 0
#endif

#ifndef RN_SKY
#define RN_SKY 0
#endif

#ifndef RN_ANIMATIONS
#define RN_ANIMATIONS 0
#endif

#if RN_UV0 && RN_ALPHA
[[vk::binding(2)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(3)]] Texture2D texture0 : register(t0);
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
#if RN_SKY
	matrix modelViewMatrix;
	matrix projectionMatrix;
#else
	matrix modelViewProjectionMatrix;
#endif

#if RN_ANIMATIONS
	matrix boneMatrices[RN_MAX_BONES];
#endif

#if RN_UV0 && RN_ALPHA
	float textureTileFactor;
#endif
};

/*
#if RN_UV0 && RN_ALPHA
[[vk::binding(2)]] cbuffer FragmentUniforms : register(b1)
{
	float2 alphaToCoverageClamp;
};
#endif*/

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;
#if RN_UV0 && RN_ALPHA
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif
#if RN_ANIMATIONS
	[[vk::location(7)]] float4 boneWeights : BONEWEIGHTS;
	[[vk::location(8)]] float4 boneIndices : BONEINDICES;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
#if RN_UV0 && RN_ALPHA
	float2 texCoords : TEXCOORD0;
#endif
};

#if RN_ANIMATIONS
float4 getAnimatedPosition(float4 position, float4 weights, float4 indices)
{
	float4 pos1 = mul(boneMatrices[int(indices.x)], position);
	float4 pos2 = mul(boneMatrices[int(indices.y)], position);
	float4 pos3 = mul(boneMatrices[int(indices.z)], position);
	float4 pos4 = mul(boneMatrices[int(indices.w)], position);

	float4 pos = pos1 * weights.x + pos2 * weights.y + pos3 * weights.z + pos4 * weights.w;
	pos.w = position.w;

	return pos;
}
#endif

FragmentVertex depth_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_SKY
	float3 rotatedPosition = mul(modelViewMatrix, float4(vert.position, 1.0f)).xyz;
	result.position = mul(projectionMatrix, float4(rotatedPosition, 1.0)).xyww;
#else
	#if RN_ANIMATIONS
		float4 position = getAnimatedPosition(float4(vert.position, 1.0), vert.boneWeights, vert.boneIndices);
	#else
		float4 position = float4(vert.position, 1.0);
	#endif
	result.position = mul(modelViewProjectionMatrix, position);
#endif

#if RN_UV0 && RN_ALPHA
	result.texCoords = vert.texCoords*textureTileFactor;
#endif

	return result;
}


void depth_fragment(
#if RN_UV0 && RN_ALPHA
	FragmentVertex vert
#endif
	)
{
#if RN_UV0 && RN_ALPHA
	float4 color = texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
	clip(color.a - 0.5f);
#endif
}
