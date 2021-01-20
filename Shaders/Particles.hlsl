//
//  Particles.hlsl
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#pragma permutator RN_UV0

#if RN_UV0
Texture2D texture0;
SamplerState linearRepeatSampler;
#endif

cbuffer vertexUniforms
{
#if RN_USE_MULTIVIEW
	matrix inverseModelViewMatrix_multiview[6];
	matrix modelViewProjectionMatrix_multiview[6];
#else
	matrix inverseModelViewMatrix;
	matrix modelViewProjectionMatrix;
#endif

	float4 cameraAmbientColor;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(3)]] float4 color : COLOR0;
#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif
	[[vk::location(6)]] float2 texCoords2 : TEXCOORD1;

#if RN_USE_MULTIVIEW
	uint viewIndex : SV_VIEWID;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
	float4 color : COLOR0;
};


FragmentVertex particles_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_USE_MULTIVIEW
	result.position = mul(modelViewProjectionMatrix_multiview[vert.viewIndex], float4(vert.position, 1.0) + mul(inverseModelViewMatrix_multiview[vert.viewIndex], float4(vert.texCoords2, 0.0, 0.0)));
#else
	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0) + mul(inverseModelViewMatrix, float4(vert.texCoords2, 0.0, 0.0)));
#endif

	result.color = vert.color*cameraAmbientColor;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

	return result;
}


float4 particles_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = vert.color;

#if RN_UV0
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
#endif

	color.rgb *= color.a;

	return color;
}
