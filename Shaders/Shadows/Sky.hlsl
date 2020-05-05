//
//  Sky.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#if RN_UV0
[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(4)]] Texture2D texture0 : register(t0);
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelViewMatrix;
	matrix projectionMatrix;
};

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 cameraAmbientColor;
	float4 diffuseColor;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;

#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif
#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR0;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;

#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif

#if RN_COLOR
	float3 color : COLOR0;
#endif
};

FragmentVertex sky_vertex(InputVertex vert)
{
	FragmentVertex result;

	float3 rotatedPosition = mul(modelViewMatrix, float4(vert.position, 1.0)).xyz;
	result.position = mul(projectionMatrix, float4(rotatedPosition, 1.0)).xyww;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

#if RN_COLOR
	result.color = vert.color.rgb;
#endif

	return result;
}


float4 sky_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = diffuseColor;

#if RN_UV0
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
#endif

#if RN_COLOR
	color.rgb *= vert.color;
#endif

	color.rgb *= cameraAmbientColor.rgb;
	return color;
}
