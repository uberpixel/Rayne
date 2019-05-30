//
//  Sky.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

Texture2D texture0 : register(t0);
SamplerState samplr : register(s0);

cbuffer vertexUniforms : register(b0)
{
	matrix modelViewMatrix;
	matrix projectionMatrix;
};

cbuffer fragmentUniforms : register(b1)
{
	float4 diffuseColor;
};

struct InputVertex
{
	float3 position : POSITION;
	float2 texCoords : TEXCOORD0;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORD0;
};

FragmentVertex sky_vertex(InputVertex vert)
{
	FragmentVertex result;

	float3 rotatedPosition = mul(modelViewMatrix, vert.position);
	result.position = mul(projectionMatrix, float4(rotatedPosition, 1.0)).xyww;
	result.texCoords = vert.texCoords;

	return result;
}


float4 sky_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = texture0.Sample(samplr, vert.texCoords).rgba;
	return color * diffuseColor;
}
