//
//  PostProcessing.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

Texture2D texture0 : register(t0);
SamplerState linearClampSampler : register(s0);

struct InputVertex
{
	float3 position : POSITION;
	float2 texCoords : TEXCOORD0;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORD0;
};

FragmentVertex pp_vertex(InputVertex vert)
{
	FragmentVertex result;
	result.position = float4(vert.position.xy, 1.0, 1.0001);
	result.texCoords = vert.texCoords;

	return result;
}

float4 pp_blit_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = texture0.Sample(linearClampSampler, vert.texCoords).rgba;
	return color;
}

FragmentVertex pp_mask_vertex(InputVertex vert)
{
	FragmentVertex result;
	result.position = float4(vert.position.xy * 2.0 - 1.0, 0.0001, 1.0);

	return result;
}

float4 pp_mask_fragment(FragmentVertex vert) : SV_TARGET
{
	return float4(1.0, 1.0, 1.0, 1.0);
}
