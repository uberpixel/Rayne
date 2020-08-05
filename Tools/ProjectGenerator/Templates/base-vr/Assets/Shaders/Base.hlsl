//
//  Base.hlsl
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#ifndef RN_ANIMATIONS
#define RN_ANIMATIONS 0
#endif

#ifndef RN_MAX_BONES
#define RN_MAX_BONES 100
#endif

#if RN_UV0
[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(4)]] Texture2D texture0 : register(t0);
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelMatrix;
	matrix modelViewProjectionMatrix;

#if RN_ANIMATIONS
	matrix boneMatrices[RN_MAX_BONES];
#endif

	float4 ambientColor;
	float4 diffuseColor;
};

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 cameraAmbientColor;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;

#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR;
#endif

#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif

#if RN_ANIMATIONS
	[[vk::location(7)]] float4 boneWeights : BONEWEIGHTS;
	[[vk::location(8)]] float4 boneIndices : BONEINDICES;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD1;

#if RN_UV0
	half2 texCoords : TEXCOORD0;
#endif
};

#if RN_ANIMATIONS
float4 getAnimatedPosition(float4 position, float4 weights, float4 indices)
{
	float4 pos1 = mul(boneMatrices[int(indices.x)], position);
	float4 pos2 = mul(boneMatrices[int(indices.y)], position);
	float4 pos3 = mul(boneMatrices[int(indices.z)], position);
	float4 pos4 = mul(boneMatrices[int(indices.w)], position);

	float4 pos = pos1 * weights.x + pos2 * weights.y + pos3 * weights.z + pos4 * weights.w;
	pos.w = position.w;

	return pos;
}
#endif

FragmentVertex main_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

#if RN_ANIMATIONS
	float4 position = getAnimatedPosition(float4(vert.position, 1.0), vert.boneWeights, vert.boneIndices);
	float4 normal = getAnimatedPosition(float4(vert.normal, 0.0), vert.boneWeights, vert.boneIndices);
#else
	float4 position = float4(vert.position, 1.0);
	float4 normal = float4(vert.normal, 0.0);
#endif

	result.position = mul(modelViewProjectionMatrix, position);

#if RN_COLOR
	result.color = vert.color * diffuseColor * ambientColor;
#else
	result.color = diffuseColor * ambientColor;
#endif

	return result;
}


half4 main_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

#if RN_UV0
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
#endif

	color.rgb *= cameraAmbientColor.rgb;
	return color;
}
