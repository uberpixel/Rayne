//
//  PostProcessing.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

struct InputVertex
{
	float3 position : POSITION;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
};

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
