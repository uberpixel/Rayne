//
//  PostProcessing.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

[[vk::binding(4)]] Texture2D texture0 : register(t0);
[[vk::binding(3)]] SamplerState linearClampSampler : register(s0);

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

FragmentVertex pp_vertex(InputVertex vert)
{
	FragmentVertex result;
	result.position = float4(vert.position.xy, 0.0, 1.0);
	result.texCoords = vert.texCoords;

	return result;
}

float4 pp_blit_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = texture0.Sample(linearClampSampler, vert.texCoords).rgba;
	return color;
}

