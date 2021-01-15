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

#include "rayne.hlsl"

#if RN_UV0
	Texture2D texture0;
	SamplerState linearRepeatSampler;
#endif
		
#if RN_NORMALS && RN_LIGHTS_DIRECTIONAL && RN_SHADOWS_DIRECTIONAL
	Texture2DArray directionalShadowTexture;
	SamplerComparisonState directionalShadowSampler;
#endif

cbuffer vertexUniforms
{
	matrix modelViewProjectionMatrix;
	matrix modelMatrix;

	RN_ANIMATION_VERTEX_UNIFORMS

#if RN_UV0
	float textureTileFactor;
#endif
};

cbuffer fragmentUniforms
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 cameraAmbientColor;

#if RN_UV0 && RN_ALPHA
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

	RN_ANIMATION_VERTEX_DATA
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

	float4 position = RN_ANIMATION_TRANSFORM(float4(vert.position, 1.0), vert)
	result.position = mul(modelViewProjectionMatrix, position);

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS && (RN_LIGHTS_DIRECTIONAL || RN_LIGHTS_POINT)
	float4 normal = RN_ANIMATION_TRANSFORM(float4(vert.normal, 0.0), vert)
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
