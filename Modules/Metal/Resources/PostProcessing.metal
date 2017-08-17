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

	float3 normal [[attribute(1)]];
	float2 texCoords [[attribute(2)]];
};

struct FragmentVertex
{
	float4 position [[position]];
	float2 texCoords;
};

vertex FragmentVertex pp_vertex(InputVertex vert [[stage_in]])
{
	FragmentVertex result;
	result.position = float4(vert.position.xy, 1.0, 1.0001).xyww;
	result.texCoords = vert.texCoords;

	return result;
}

fragment float4 pp_blit_fragment(FragmentVertex vert [[stage_in]], texture2d<float> texture0 [[texture(0)]], sampler linearClampSampler [[sampler(0)]])
{
	float4 color = texture0.sample(linearClampSampler, vert.texCoords).rgba;
	return color;
}
