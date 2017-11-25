//
//  Shaders.metal
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <metal_stdlib>
#include <simd/simd.h>

// Exported defines:
// RN_NORMALS
// RN_COLOR
// RN_UV0
// RN_ALPHA

using namespace metal;

#ifndef RN_NORMALS
#define RN_NORMALS 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#ifndef RN_UV0
#define RN_UV0 0
#endif


struct LightDirectional
{
	packed_float3 direction;
	packed_float4 color;
};

struct Uniforms
{
	matrix_float4x4 viewProjectionMatrix;
	matrix_float4x4 modelViewProjectionMatrix;
	matrix_float4x4 modelMatrix;

#if RN_UV0
	float textureTileFactor;
#endif
};

struct FragmentUniforms
{
	float4 ambientColor;
	float4 diffuseColor;

#if RN_ALPHA
	float2 alphaToCoverageClamp;
#endif
	
	short directionalShadowMatricesCount;
	float2 directionalShadowInfo;
	matrix_float4x4 directionalShadowMatrices[4];

	short directionalLightsCount;
	LightDirectional directionalLights[5];
};


struct InputVertex
{
	float3 position [[attribute(0)]];

#if RN_NORMALS
	float3 normal [[attribute(1)]];
#endif
#if RN_COLOR
	float4 color [[attribute(3)]];
#endif
#if RN_UV0
	float2 texCoords [[attribute(5)]];
#endif
#if RN_TANGENTS
	float4 tangents [[attribute(2)]];
#endif
};

struct FragmentVertex
{
	float4 position [[position]];
	float3 worldPosition;

#if RN_COLOR
	float4 color;
#endif
#if RN_NORMALS
	float3 normal;
#endif
#if RN_UV0
	float2 texCoords;
#endif
};

float getShadowPCF(float4 projected, float2 offset, float2 shadowInfo, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler);
float getShadowPCF(float4 projected, float2 offset, float2 shadowInfo, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler)
{
	return directionalShadowTexture.sample_compare(directionalShadowSampler, projected.xy + offset * shadowInfo, projected.z, projected.w);
}

//basic 2x2 blur, with hardware bilinear filtering if enabled
float getShadowPCF2x2(float4 projected, float2 shadowInfo, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler);
float getShadowPCF2x2(float4 projected, float2 shadowInfo, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler)
{
	float shadow = getShadowPCF(projected, float2(0.0, 0.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(1.0, 0.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(0.0, 1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(1.0, 1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow *= 0.25f;
	return shadow;
}

//basic 4x4 blur, with hardware bilinear filtering if enabled
float getShadowPCF4x4(float4 projected, float2 shadowInfo, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler);
float getShadowPCF4x4(float4 projected, float2 shadowInfo, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler)
{
	float shadow = getShadowPCF(projected, float2(-2.0, -2.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(-1.0, -2.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(0.0, -2.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(1.0, -2.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	
	shadow += getShadowPCF(projected, float2(-2.0, -1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(-1.0, -1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(0.0, -1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(1.0, -1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	
	shadow += getShadowPCF(projected, float2(-2.0, 0.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(-1.0, 0.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(0.0, 0.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(1.0, 0.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	
	shadow += getShadowPCF(projected, float2(-2.0, 1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(-1.0, 1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(0.0, 1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	shadow += getShadowPCF(projected, float2(1.0, 1.0), shadowInfo, directionalShadowTexture, directionalShadowSampler);
	
	shadow *= 0.0625;
	return shadow;
}

float getDirectionalShadowFactor(short light, float3 position, constant FragmentUniforms &uniforms, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler);
float getDirectionalShadowFactor(short light, float3 position, constant FragmentUniforms &uniforms, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler)
{
	if(light != 0 || uniforms.directionalShadowMatricesCount == 0)
		return 1.0f;

	float4 projectedPosition[4];
	short mapToUse = -1;
	for(short i = 0; i < uniforms.directionalShadowMatricesCount; i++)
	{
		projectedPosition[i] = uniforms.directionalShadowMatrices[i] * float4(position, 1.0);
		projectedPosition[i].xyz /= projectedPosition[i].w;

		if(mapToUse < 0 && abs(projectedPosition[i].x) < 1.0f && abs(projectedPosition[i].y) < 1.0f && abs(projectedPosition[i].z) < 1.0f)
		{
			mapToUse = i;
		}
	}

	if(mapToUse == -1)
		return 1.0;
	
	projectedPosition[mapToUse].y *= -1.0f;
	projectedPosition[mapToUse].xy *= 0.5f;
	projectedPosition[mapToUse].xy += 0.5f;
	projectedPosition[mapToUse].w = mapToUse;

	if(mapToUse < 3)
		return getShadowPCF4x4(projectedPosition[mapToUse].xywz, uniforms.directionalShadowInfo, directionalShadowTexture, directionalShadowSampler);
	else
		return getShadowPCF2x2(projectedPosition[mapToUse].xywz, uniforms.directionalShadowInfo, directionalShadowTexture, directionalShadowSampler);
}

float4 getDirectionalLights(float3 position, float3 normal, constant FragmentUniforms &uniforms, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler);
float4 getDirectionalLights(float3 position, float3 normal, constant FragmentUniforms &uniforms, depth2d_array<float> directionalShadowTexture, sampler directionalShadowSampler)
{
	float4 light = 0.0f;
	for(short i = 0; i < uniforms.directionalLightsCount; i++)
	{
		light += saturate(dot(normal, -uniforms.directionalLights[i].direction)) * uniforms.directionalLights[i].color * getDirectionalShadowFactor(i, position, uniforms, directionalShadowTexture, directionalShadowSampler);
	}
	light.a = 1.0f;
	return light;
}


// Non instanced

vertex FragmentVertex gouraud_vertex(const InputVertex vert [[stage_in]], constant Uniforms &uniforms [[buffer(1)]])
{
	FragmentVertex result;

	result.position = uniforms.modelViewProjectionMatrix * float4(vert.position, 1.0);
	result.worldPosition = (uniforms.modelMatrix * float4(vert.position, 1.0f)).xyz;

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS
	result.normal = (uniforms.modelMatrix * float4(vert.normal, 0.0)).xyz;
#endif
#if RN_UV0
	result.texCoords = vert.texCoords * uniforms.textureTileFactor;
#endif

	return result;
}


fragment
#if !RN_ALPHA
[[early_fragment_tests]]
#endif
float4 gouraud_fragment(FragmentVertex vert [[stage_in]]
#if RN_UV0
	, texture2d<float> texture [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]]
	, depth2d_array<float> directionalShadowTexture [[texture(1)]], sampler directionalShadowSampler [[sampler(1)]]
#else
	, depth2d_array<float> directionalShadowTexture [[texture(0)]], sampler directionalShadowSampler [[sampler(0)]]
#endif
	, constant FragmentUniforms &uniforms [[buffer(1)]]
)
{
	float4 color = uniforms.diffuseColor;
#if RN_UV0
	color *= texture.sample(linearRepeatSampler, vert.texCoords).rgba;

#if RN_ALPHA
	color.a = smoothstep(uniforms.alphaToCoverageClamp.x, uniforms.alphaToCoverageClamp.y, color.a);
	if(color.a < 0.001)
		return color;
#endif
#endif

#if RN_COLOR
	color *= vert.color;
#endif

#if RN_NORMALS
	float4 light = getDirectionalLights(vert.worldPosition, normalize(vert.normal), uniforms, directionalShadowTexture, directionalShadowSampler);
	return color * (uniforms.ambientColor + light);
#else
	return color * (uniforms.ambientColor);
#endif
}

// Instancing

struct InstanceBufferUniform
{
	int index;
};

struct InstanceBufferMatrixUniform
{
	matrix_float4x4 _modelMatrix;
	matrix_float4x4 _inverseModelMatrix;
};

vertex FragmentVertex gouraud_vertex_instanced(const InputVertex vert [[stage_in]], constant Uniforms &uniforms [[buffer(1)]], constant InstanceBufferUniform *instanceUniforms [[buffer(2)]], constant InstanceBufferMatrixUniform *matricesUniform [[buffer(3)]], ushort iid [[instance_id]])
{
	int index = instanceUniforms[iid].index;

	FragmentVertex result;

	result.position = uniforms.viewProjectionMatrix * matricesUniform[index]._modelMatrix * float4(vert.position, 1.0);

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS
	result.normal = (uniforms.modelMatrix * float4(vert.normal, 0.0)).xyz;
#endif
#if RN_UV0
	result.texCoords = vert.texCoords * uniforms.textureTileFactor;
#endif

	return result;
}

fragment
#if !RN_ALPHA
[[early_fragment_tests]]
#endif
float4 gouraud_fragment_instanced(FragmentVertex vert [[stage_in]]
#if RN_UV0
	, texture2d<float> texture [[texture(0)]], sampler samplr [[sampler(0)]]
#endif
	, constant FragmentUniforms &uniforms [[buffer(1)]]
)
{
	float4 color = uniforms.diffuseColor;
#if RN_UV0
	color *= texture.sample(samplr, vert.texCoords).rgba;

#if RN_ALPHA
	color.a = smoothstep(uniforms.alphaToCoverageClamp.x, uniforms.alphaToCoverageClamp.y, color.a);
	if(color.a < 0.001)
		return color;
#endif
#endif

#if RN_COLOR
	color *= vert.color;
#endif

#if RN_NORMALS
	float4 light = float4(1.0);//getDirectionalLights(normalize(vert.normal), uniforms.directionalLightsCount, uniforms.directionalLights);
	return color * (uniforms.ambientColor + light);
#else
	return color * (uniforms.ambientColor);
#endif
}

