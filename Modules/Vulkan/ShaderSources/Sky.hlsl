//
//  Sky.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

[[vk::binding(4)]] Texture2D texture0 : register(t0);
[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelViewMatrix;
	matrix projectionMatrix;
};

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 diffuseColor;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORD0;
};

FragmentVertex sky_vertex(InputVertex vert)
{
	FragmentVertex result;

	float3 rotatedPosition = mul(modelViewMatrix, float4(vert.position, 1.0)).xyz;
	result.position = mul(projectionMatrix, float4(rotatedPosition, 1.0)).xyww;
	result.texCoords = vert.texCoords;

	return result;
}


float4 sky_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
	return color * diffuseColor;
}
