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
	matrix inverseModelViewMatrix;
	matrix modelViewProjectionMatrix;

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

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0) + mul(inverseModelViewMatrix, float4(vert.texCoords2, 0.0, 0.0)));
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
