//
//  Shaders.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct InputVertex
{
	float3 position [[attribute(0)]];
	float2 texCoords [[attribute(5)]];
};

struct FragmentVertex
{
	float4 position [[position]];
	float2 texCoords;
};

vertex FragmentVertex pp_vertex(const InputVertex vert [[stage_in]])
{
	FragmentVertex result;
	result.position = float4(vert.position.xy, 1.0, 1.0001);
	result.texCoords = vert.texCoords;

	return result;
}

fragment [[early_fragment_tests]] float4 pp_blit_fragment(FragmentVertex vert [[stage_in]], texture2d<float> texture0 [[texture(0)]], sampler linearClampSampler [[sampler(0)]])
{
	float4 color = texture0.sample(linearClampSampler, vert.texCoords).rgba;
	return color;
}

vertex FragmentVertex pp_mask_vertex(const InputVertex vert [[stage_in]])
{
	FragmentVertex result;
	result.position = float4(vert.position.xy * 2.0 - 1.0, 0.0001, 1.0);

	return result;
}

fragment [[early_fragment_tests]] float4 pp_mask_fragment(FragmentVertex vert [[stage_in]])
{
	return float4(1.0);
}
