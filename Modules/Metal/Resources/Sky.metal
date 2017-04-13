//
//  Sky.metal
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct SkyUniforms
{
	matrix_float4x4 projectionMatrix;
	matrix_float4x4 modelViewMatrix;

	float4 diffuseColor;
};

struct SkyInputVertex
{
	float3 position [[attribute(0)]];
	float2 normal [[attribute(1)]];
	float2 texCoords [[attribute(2)]];
};

struct SkyFragmentVertex
{
	float4 position [[position]];

	float2 texCoords;
	float4 diffuse;
};

vertex SkyFragmentVertex sky_vertex(SkyInputVertex vert [[stage_in]], constant SkyUniforms &uniforms [[buffer(1)]])
{
	SkyFragmentVertex result;

	float3 rotatedPosition = (matrix_float3x3(uniforms.modelViewMatrix[0].xyz, uniforms.modelViewMatrix[1].xyz, uniforms.modelViewMatrix[2].xyz) * vert.position).xyz;
	result.position = (uniforms.projectionMatrix * float4(rotatedPosition, 1.0)).xyww;
	result.texCoords = vert.texCoords;
	result.diffuse = uniforms.diffuseColor;

	return result;
}

fragment float4 sky_fragment(SkyFragmentVertex vert [[stage_in]], texture2d<float> texture0 [[texture(0)]], sampler sampler0 [[sampler(0)]])
{
	float4 color = texture0.sample(sampler0, vert.texCoords).rgba;
	return color * vert.diffuse;
}
