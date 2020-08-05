//
//  PostProcessing.hlsl
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(4)]] Texture2D texture0 : register(t0);

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	float4 diffuseColor;
	float4 ambientColor;
};

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
	result.position = float4(vert.position, 1.0f).xyww;
	result.texCoords = vert.texCoords * diffuseColor.zw + diffuseColor.xy;

	return result;
}

float4 pp_blit_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = texture0.Sample(linearRepeatSampler, vert.texCoords).rgba * ambientColor.rgba;
	return color;
}
