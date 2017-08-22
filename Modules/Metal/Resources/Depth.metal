//
//  Depth.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Exported defines:
// RN_UV0
// RN_ALPHA
// RN_SKY

#include <metal_stdlib>
#include <simd/simd.h>

#ifndef RN_ALPHA
#define RN_ALPHA 0
#endif

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_SKY
#define RN_SKY 0
#endif

using namespace metal;

struct VertexUniforms
{
#if RN_SKY
	matrix_float4x4 modelViewMatrix;
	matrix_float4x4 projectionMatrix;
#else
	matrix_float4x4 modelViewProjectionMatrix;
#endif

#if RN_UV0 && RN_ALPHA
	float textureTileFactor;
#endif
};

#if RN_UV0 && RN_ALPHA
struct FragmentUniforms
{
#if RN_ALPHA
	float discardThreshold;
#endif
};
#endif

struct InputVertex
{
	float3 position [[attribute(0)]];
#if RN_UV0 && RN_ALPHA
	float2 texCoords [[attribute(1)]];
#endif
};

struct FragmentVertex
{
	float4 position [[position]];
#if RN_UV0 && RN_ALPHA
	float2 texCoords;
#endif
};

vertex FragmentVertex depth_vertex(InputVertex vert [[stage_in]], constant VertexUniforms &uniforms [[buffer(1)]])
{
	FragmentVertex result;

#if RN_SKY
	float3 rotatedPosition = (uniforms.modelViewMatrix * float4(vert.position, 0.0)).xyz;
	result.position = (uniforms.projectionMatrix * float4(rotatedPosition, 1.0)).xyww;
#else
	result.position = uniforms.modelViewProjectionMatrix * float4(vert.position, 1.0f);
#endif

#if RN_UV0 && RN_ALPHA
	result.texCoords = vert.texCoords*uniforms.textureTileFactor;
#endif

	return result;
}


fragment float4 depth_fragment(FragmentVertex vert [[stage_in]]
#if RN_UV0 && RN_ALPHA
	, texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]]
	, constant FragmentUniforms &uniforms [[buffer(1)]]
#endif
)
{
	float4 color = 1.0f;
#if RN_UV0 && RN_ALPHA
	color *= texture0.sample(linearRepeatSampler, vert.texCoords).rgba;
#endif
	return color;
}
