//
//  PostProcessing.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_PP_VR
#define RN_PP_VR 0
#endif

#if RN_PP_VR
	Texture2DArray framebufferTexture;
#else
	Texture2D framebufferTexture;
#endif

SamplerState linearClampSampler;

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
	result.position = float4(vert.position.xy, 0.0, 1.0);
	result.texCoords = vert.texCoords;

	return result;
}

float4 pp_blit_fragment(FragmentVertex vert) : SV_TARGET
{
#if RN_PP_VR
	float4 color = framebufferTexture.Sample(linearClampSampler, float3(frac(vert.texCoords * float2(2.0, 1.0)), vert.texCoords.x > 0.5? 1.0:0.0)).rgba;
#else
	float4 color = framebufferTexture.Sample(linearClampSampler, vert.texCoords).rgba;
#endif
	return color;
}

