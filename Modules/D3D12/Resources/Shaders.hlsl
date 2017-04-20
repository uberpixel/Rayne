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
// RN_DISCARD

#if RN_UV0
Texture2D texture0 : register(t0);
SamplerState linearRepeatSampler : register(s0);

Texture2DArray directionalShadowTexture : register(t1);
SamplerComparisonState directionalShadowSampler : register(s1);
#else
Texture2DArray directionalShadowTexture : register(t0);
SamplerComparisonState directionalShadowSampler : register(s0);
#endif

cbuffer vertexUniforms : register(b0)
{
	matrix modelViewProjectionMatrix;
	matrix modelMatrix;

#if RN_UV0
	float textureTileFactor;
#endif
};

struct LightDirectional
{
	float4 direction;
	float4 color;
};

cbuffer fragmentUniforms : register(b1)
{
	float4 ambientColor;
	float4 diffuseColor;

#if RN_DISCARD
	float discardThreshold;
#endif

	uint directionalShadowMatricesCount;
	matrix directionalShadowMatrices[4];

	uint directionalLightsCount;
	LightDirectional directionalLights[5];
};

struct InputVertex
{
	float3 position : POSITION;

#if RN_NORMALS
	float3 normal : NORMAL;
#endif
#if RN_COLOR
	float4 color : COLOR0;
#endif
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
#if RN_TANGENTS
	float4 tangents : TANGENT;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	float3 worldPosition : POSITION;

#if RN_NORMALS
	float3 normal : NORMAL;
#endif
#if RN_COLOR
	float4 color : COLOR0;
#endif
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
};

float getDirectionalShadowFactor(int light, float3 position)
{
	if(light != 0 || directionalShadowMatricesCount == 0)
		return 1.0f;

	float4 projectedPosition[4];
	uint mapToUse = -1;
	for(int i = 0; i < directionalShadowMatricesCount; i++)
	{
		projectedPosition[i] = mul(directionalShadowMatrices[i], float4(position, 1.0));
		projectedPosition[i].xyz /= projectedPosition[i].w;

		if(mapToUse > i && abs(projectedPosition[i].x) < 1.0f && abs(projectedPosition[i].y) < 1.0f)
		{
			mapToUse = i;
		}
	}
	
	projectedPosition[mapToUse].y *= -1.0f;
	projectedPosition[mapToUse].xyz *= 0.5f;
	projectedPosition[mapToUse].xyz += 0.5f;
	projectedPosition[mapToUse].w = projectedPosition[mapToUse].z;
	projectedPosition[mapToUse].z = mapToUse;

	return directionalShadowTexture.SampleCmp(directionalShadowSampler, projectedPosition[mapToUse].xyz, projectedPosition[mapToUse].w+0.001f);
}

float4 getDirectionalLights(float3 position, float3 normal, uint count, LightDirectional directionalLights[5])
{
	float4 light = 0.0f;
	for(uint i = 0; i < count; i++)
	{
		light += saturate(dot(normal, -directionalLights[i].direction.xyz)) * directionalLights[i].color * getDirectionalShadowFactor(i, position);
	}
	light.a = 1.0f;
	return light;
}

FragmentVertex gouraud_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0f));
	result.worldPosition = mul(modelMatrix, float4(vert.position, 1.0f));

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS
	result.normal = mul(modelMatrix, float4(vert.normal, 0.0f)).xyz;
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

#if RN_DISCARD
	clip(color.a - discardThreshold);
#endif
#endif

#if RN_COLOR
	color *= vert.color;
#endif

#if RN_NORMALS
	float4 light = getDirectionalLights(vert.worldPosition, normalize(vert.normal), directionalLightsCount, directionalLights);
	return color * (ambientColor + light);
#else
	return color * (ambientColor);
#endif
}
