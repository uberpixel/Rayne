//
//  Shaders.hlsl
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Exported defines:
// RN_NORMALS
// RN_COLOR
// RN_UV0
// RN_ALPHA


#ifndef RN_MAX_BONES
#define RN_MAX_BONES 100
#endif

#ifndef RN_MAX_POINTLIGHTS
#define RN_MAX_POINTLIGHTS 1
#endif


#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#ifndef RN_NORMALS
#define RN_NORMALS 0
#endif

#ifndef RN_ALPHA
#define RN_ALPHA 0
#endif

#ifndef RN_ANIMATIONS
#define RN_ANIMATIONS 0
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

#if RN_UV0
	[[vk::binding(5)]] Texture2D texture0 : register(t0);
	[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);

	#if RN_NORMALS && RN_LIGHTS_DIRECTIONAL && RN_SHADOWS_DIRECTIONAL
		[[vk::binding(6)]] Texture2DArray directionalShadowTexture : register(t1);
		[[vk::binding(4)]] SamplerComparisonState directionalShadowSampler : register(s1);
	#endif
#elif RN_NORMALS && RN_LIGHTS_DIRECTIONAL && RN_SHADOWS_DIRECTIONAL
	[[vk::binding(4)]] Texture2DArray directionalShadowTexture : register(t0);
	[[vk::binding(3)]] SamplerComparisonState directionalShadowSampler : register(s0);
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelViewProjectionMatrix;
	matrix modelMatrix;

#if RN_ANIMATIONS
	matrix boneMatrices[RN_MAX_BONES];
#endif

#if RN_UV0
	float textureTileFactor;
#endif
};

#if RN_LIGHTS_DIRECTIONAL
struct LightDirectional
{
	float4 direction;
	float4 color;
};
#endif

#if RN_LIGHTS_POINT
struct PointLight
{
	float4 positionAndRange;
	float4 color;
};
#endif

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 cameraAmbientColor;

#if RN_ALPHA
	float2 alphaToCoverageClamp;
#endif

#if RN_NORMALS
	#if RN_LIGHTS_POINT
		PointLight pointLights[RN_MAX_POINTLIGHTS];
	#endif

	#if RN_LIGHTS_DIRECTIONAL
		#if RN_SHADOWS_DIRECTIONAL
			uint directionalShadowMatricesCount;
		#endif
		uint directionalLightsCount;
		
		#if RN_SHADOWS_DIRECTIONAL
			float2 directionalShadowInfo;
			matrix directionalShadowMatrices[4];
		#endif

		LightDirectional directionalLights[5];
	#endif
#endif
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;

#if RN_NORMALS && (RN_LIGHTS_DIRECTIONAL || RN_LIGHTS_POINT)
	[[vk::location(1)]] float3 normal : NORMAL;
#endif
#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR0;
#endif
#if RN_UV0
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

#if RN_NORMALS && (RN_LIGHTS_DIRECTIONAL || RN_LIGHTS_POINT)
	float3 normal : NORMAL;
	float3 worldPosition : POSITION;
#endif
#if RN_COLOR
	float4 color : COLOR0;
#endif
#if RN_UV0
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

#if RN_NORMALS && RN_LIGHTS_DIRECTIONAL && RN_SHADOWS_DIRECTIONAL
float getShadowPCF(float4 projected, float2 offset)
{
	return directionalShadowTexture.SampleCmpLevelZero(directionalShadowSampler, float3(projected.xy + offset * directionalShadowInfo, projected.z), projected.w);
}

//basic 2x2 blur, with hardware bilinear filtering if enabled
float getShadowPCF2x2(float4 projected)
{
	float shadow = getShadowPCF(projected, float2(0.0, 0.0));
	shadow += getShadowPCF(projected, float2(1.0, 0.0));
	shadow += getShadowPCF(projected, float2(0.0, 1.0));
	shadow += getShadowPCF(projected, float2(1.0, 1.0));
	shadow *= 0.25f;
	return shadow;
}

//basic 4x4 blur, with hardware bilinear filtering if enabled
float getShadowPCF4x4(float4 projected)
{
	float shadow = getShadowPCF(projected, float2(-2.0, -2.0));
	shadow += getShadowPCF(projected, float2(-1.0, -2.0));
	shadow += getShadowPCF(projected, float2(0.0, -2.0));
	shadow += getShadowPCF(projected, float2(1.0, -2.0));
	
	shadow += getShadowPCF(projected, float2(-2.0, -1.0));
	shadow += getShadowPCF(projected, float2(-1.0, -1.0));
	shadow += getShadowPCF(projected, float2(0.0, -1.0));
	shadow += getShadowPCF(projected, float2(1.0, -1.0));
	
	shadow += getShadowPCF(projected, float2(-2.0, 0.0));
	shadow += getShadowPCF(projected, float2(-1.0, 0.0));
	shadow += getShadowPCF(projected, float2(0.0, 0.0));
	shadow += getShadowPCF(projected, float2(1.0, 0.0));
	
	shadow += getShadowPCF(projected, float2(-2.0, 1.0));
	shadow += getShadowPCF(projected, float2(-1.0, 1.0));
	shadow += getShadowPCF(projected, float2(0.0, 1.0));
	shadow += getShadowPCF(projected, float2(1.0, 1.0));
	
	shadow *= 0.0625;
	return shadow;
}

float getDirectionalShadowFactor(int light, float3 position)
{
	if(light != 0 || directionalShadowMatricesCount == 0)
		return 1.0f;

	float4 projectedPosition[4];
	uint mapToUse = -1;
	for(uint i = 0; i < directionalShadowMatricesCount; i++)
	{
		projectedPosition[i] = mul(directionalShadowMatrices[i], float4(position, 1.0));
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
		return getShadowPCF2x2(projectedPosition[mapToUse].xywz);
	else
		return getShadowPCF4x4(projectedPosition[mapToUse].xywz);
}
#endif

#if RN_NORMALS && RN_LIGHTS_DIRECTIONAL
float4 getDirectionalLights(float3 position, float3 normal, uint count, LightDirectional directionalLights[5])
{
	float4 light = 0.0f;
	for(uint i = 0; i < count; i++)
	{
		float4 currentLight = saturate(dot(normal, -directionalLights[i].direction.xyz)) * directionalLights[i].color;
		#if RN_SHADOWS_DIRECTIONAL
		currentLight *= getDirectionalShadowFactor(i, position);
		#endif

		light += currentLight;
	}
	light.a = 1.0;
	return light;
}
#endif

#if RN_NORMALS && RN_LIGHTS_POINT
float4 getPointLights(float3 position, float3 normal, PointLight pointLights[RN_MAX_POINTLIGHTS])
{
	float4 light = 0.0f;
	for(uint i = 0; i < RN_MAX_POINTLIGHTS; i++)
	{
		float3 direction = pointLights[i].positionAndRange.xyz - position;
		float attenuation = 1.0 / (1.0 + dot(direction, direction) / (pointLights[i].positionAndRange.w * pointLights[i].positionAndRange.w));
		light += saturate(dot(normal, normalize(direction))) * pointLights[i].color * attenuation;
	}
	light.a = 1.0;
	return light;
}
#endif

FragmentVertex gouraud_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_ANIMATIONS
	float4 position = getAnimatedPosition(float4(vert.position, 1.0), vert.boneWeights, vert.boneIndices);
#else
	float4 position = float4(vert.position, 1.0);
#endif

	result.position = mul(modelViewProjectionMatrix, position);

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS && (RN_LIGHTS_DIRECTIONAL || RN_LIGHTS_POINT)
	#if RN_ANIMATIONS
		float4 normal = getAnimatedPosition(float4(vert.normal, 0.0), vert.boneWeights, vert.boneIndices);
	#else
		float4 normal = float4(vert.normal, 0.0);
	#endif

	result.normal = mul(modelMatrix, normal).xyz;
	result.worldPosition = mul(modelMatrix, position).xyz;
#endif
#if RN_UV0
	result.texCoords = vert.texCoords*textureTileFactor;
#endif

	return result;
}


float4 gouraud_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = diffuseColor;
#if RN_UV0
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;

	#if RN_ALPHA
		color.a = smoothstep(alphaToCoverageClamp.x, alphaToCoverageClamp.y, color.a);
		if(color.a < 0.001)
			return color;
	#endif
#endif

#if RN_COLOR
	color *= vert.color;
#endif

#if RN_NORMALS && (RN_LIGHTS_DIRECTIONAL || RN_LIGHTS_POINT)
	float4 light = 0.0;
	#if RN_LIGHTS_DIRECTIONAL
		light += getDirectionalLights(vert.worldPosition, normalize(vert.normal), directionalLightsCount, directionalLights);
	#endif
	#if RN_LIGHTS_POINT
		light += getPointLights(vert.worldPosition, normalize(vert.normal), pointLights);
	#endif

	return color * (ambientColor + light) * cameraAmbientColor;
#else
	return color * (ambientColor) * cameraAmbientColor;
#endif
}
