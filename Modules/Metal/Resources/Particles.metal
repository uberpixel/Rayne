//
//  Particles.metal
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
//	matrix_float4x4 modelMatrix;
	matrix_float4x4 inverseViewMatrix;
//	matrix_float4x4 modelProjectionMatrix;
	matrix_float4x4 modelViewProjectionMatrix;
};


struct InputVertex
{
	float3 position [[attribute(0)]];
	float4 color [[attribute(3)]];
	float2 texCoords [[attribute(5)]];
	float2 texCoords2 [[attribute(6)]];
};

struct FragmentVertex
{
	float4 position [[position]];

	float4 color;
//	float2 texCoords;
};

// Non instanced

vertex FragmentVertex particles_vertex(const InputVertex vert [[stage_in]], constant Uniforms &uniforms [[buffer(1)]])
{
	FragmentVertex result;

	result.position = uniforms.modelViewProjectionMatrix * (float4(vert.position, 1.0) + uniforms.inverseViewMatrix * float4(vert.texCoords2, 0.0, 0.0));
	result.color = vert.color;

//	result.texCoords = vert.texCoords;

	return result;
}

fragment
float4 particles_fragment(FragmentVertex vert [[stage_in]]
//	, texture2d<float> texture [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]]
)
{
//	float4 color = texture.sample(linearRepeatSampler, vert.texCoords).rgba;
//	color *= vert.color;

	float4 color = vert.color;
	color.rgb *= color.a;

	return color;
}


