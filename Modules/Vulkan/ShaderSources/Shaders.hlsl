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

#if RN_UV0
[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(4)]] Texture2D texture0 : register(t0);
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
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

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 ambientColor;
	float4 diffuseColor;

#if RN_ALPHA
	float2 alphaToCoverageClamp;
#endif

	float spacer;
	uint directionalLightsCount;
	LightDirectional directionalLights[5];
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;

#if RN_NORMALS
	[[vk::location(1)]] float3 normal : NORMAL;
#endif
#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR0;
#endif
#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
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

float4 getDirectionalLights(float3 position, float3 normal, uint count, LightDirectional directionalLights[5])
{
	float4 light = 0.0f;
	for(uint i = 0; i < 1; i++)
	{
		light += saturate(dot(normal, -directionalLights[i].direction.xyz)) * directionalLights[i].color;
	}
	light.a = 1.0f;
	return light;
}

FragmentVertex gouraud_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0f));
	result.worldPosition = mul(modelMatrix, float4(vert.position, 1.0f)).xyz;

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

#if RN_ALPHA
	color.a = smoothstep(alphaToCoverageClamp.x, alphaToCoverageClamp.y, color.a);
	if(color.a < 0.001)
		return color;
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
