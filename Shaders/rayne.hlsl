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

#ifndef RN_USE_MULTIVIEW
#define RN_USE_MULTIVIEW 0
#endif

/*
#if RN_USE_MULTIVIEW
#define MAX_MULTIVIEW_COUNT 2
#define ModelViewMatrix ModelViewMatrix[MAX_MULTIVIEW_COUNT]
#define ModelViewProjectionMatrix ModelViewProjectionMatrix[MAX_MULTIVIEW_COUNT]
#define ViewMatrix ViewMatrix[MAX_MULTIVIEW_COUNT]
#define ViewProjectionMatrix ViewProjectionMatrix[MAX_MULTIVIEW_COUNT]
#define ProjectionMatrix ProjectionMatrix[MAX_MULTIVIEW_COUNT]
#define InverseModelViewMatrix InverseModelViewMatrix[MAX_MULTIVIEW_COUNT]
#define InverseModelViewProjectionMatrix InverseModelViewProjectionMatrix[MAX_MULTIVIEW_COUNT]
#define InverseViewMatrix InverseViewMatrix[MAX_MULTIVIEW_COUNT]
#define InverseViewProjectionMatrix InverseViewProjectionMatrix[MAX_MULTIVIEW_COUNT]
#define InverseProjectionMatrix InverseProjectionMatrix[MAX_MULTIVIEW_COUNT]
#define CameraPosition CameraPosition[MAX_MULTIVIEW_COUNT]
#define CameraAmbientColor CameraAmbientColor[MAX_MULTIVIEW_COUNT]

				ModelViewMatrix
				ModelViewProjectionMatrix,
				ViewMatrix,
				ViewProjectionMatrix,
				ProjectionMatrix,
				InverseModelViewMatrix,
				InverseModelViewProjectionMatrix,
				InverseViewMatrix,
				InverseViewProjectionMatrix,
				InverseProjectionMatrix,
				CameraPosition,
				CameraAmbientColor
#endif*/

#if RN_ANIMATIONS
#define RN_ANIMATION_VERTEX_UNIFORMS matrix boneMatrices[RN_MAX_BONES];
#define RN_ANIMATION_VERTEX_DATA  \
	[[vk::location(7)]] float4 boneWeights : BONEWEIGHTS; \
	[[vk::location(8)]] float4 boneIndices : BONEINDICES;
#define RN_ANIMATION_TRANSFORM(position, vertex) \
	float4( \
		mul(boneMatrices[int(vertex.boneIndices.x)], position).xyz * vertex.boneWeights.x + \
		mul(boneMatrices[int(vertex.boneIndices.y)], position).xyz * vertex.boneWeights.y + \
		mul(boneMatrices[int(vertex.boneIndices.z)], position).xyz * vertex.boneWeights.z + \
		mul(boneMatrices[int(vertex.boneIndices.w)], position).xyz * vertex.boneWeights.w, position.w);
#else
#define RN_ANIMATION_VERTEX_UNIFORMS
#define RN_ANIMATION_VERTEX_DATA
#define RN_ANIMATION_TRANSFORM(position, vertex) position;
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

#if RN_NORMALS && RN_LIGHTS_DIRECTIONAL && RN_SHADOWS_DIRECTIONAL
Texture2DArray directionalShadowTexture;
SamplerComparisonState directionalShadowSampler;

float getShadowPCF(float4 projected, float2 offset)
{
	return directionalShadowTexture.SampleCmpLevelZero(directionalShadowSampler, float3(projected.xy + offset, projected.z), projected.w);
}

//basic 2x2 blur, with hardware bilinear filtering if enabled
float getShadowPCF2x2(float4 projected, float2 shadowInfo)
{
	float shadow = getShadowPCF(projected, float2(0.0, 0.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(1.0, 0.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(0.0, 1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(1.0, 1.0) * shadowInfo);
	shadow *= 0.25f;
	return shadow;
}

//basic 4x4 blur, with hardware bilinear filtering if enabled
float getShadowPCF4x4(float4 projected, float2 shadowInfo)
{
	float shadow = getShadowPCF(projected, float2(-2.0, -2.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(-1.0, -2.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(0.0, -2.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(1.0, -2.0) * shadowInfo);
	
	shadow += getShadowPCF(projected, float2(-2.0, -1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(-1.0, -1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(0.0, -1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(1.0, -1.0) * shadowInfo);
	
	shadow += getShadowPCF(projected, float2(-2.0, 0.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(-1.0, 0.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(0.0, 0.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(1.0, 0.0) * shadowInfo);
	
	shadow += getShadowPCF(projected, float2(-2.0, 1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(-1.0, 1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(0.0, 1.0) * shadowInfo);
	shadow += getShadowPCF(projected, float2(1.0, 1.0) * shadowInfo);
	
	shadow *= 0.0625;
	return shadow;
}

float getDirectionalShadowFactor(int light, float3 position, matrix shadowMatrices[4], int matrixCount, float2 shadowInfo)
{
	if(light != 0 || matrixCount == 0)
		return 1.0f;

	float4 projectedPosition[4];
	uint mapToUse = -1;
	for(uint i = 0; i < matrixCount; i++)
	{
		projectedPosition[i] = mul(shadowMatrices[i], float4(position, 1.0));
		projectedPosition[i].xyz /= projectedPosition[i].w;

		if(mapToUse > i && abs(projectedPosition[i].x) < 1.0 && abs(projectedPosition[i].y) < 1.0 && abs(projectedPosition[i].z) < 1.0)
		{
			mapToUse = i;
		}
	}

	if(mapToUse == -1)
		return 1.0f;
	
	projectedPosition[mapToUse].y *= -1.0;
	projectedPosition[mapToUse].xy *= 0.5;
	projectedPosition[mapToUse].xy += 0.5;
	projectedPosition[mapToUse].w = mapToUse;

	if(mapToUse < 3)
		return getShadowPCF2x2(projectedPosition[mapToUse].xywz, shadowInfo);
	else
		return getShadowPCF4x4(projectedPosition[mapToUse].xywz, shadowInfo);
}
#endif



