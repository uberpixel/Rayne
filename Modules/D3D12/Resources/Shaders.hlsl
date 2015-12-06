//
//  Shaders.hlsl
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

Texture2D texture : register(t0);
SamplerState sampler : register(s0);

struct Vertex
{
	float2 position : POSITION;
	float2 texCoords : TEXCOORD0;
};

struct VertexResult
{
	float4 position : SV_POSITION;
	float2 texCoords : TEXCOORD0;
};

VertexResult basic_blit_vertex(Vertex vert)
{
	VertexResult result;
	result.position = float4(vert.position, 0.0, 1.0);
	result.texCoords = vert.texCoords;

	return result;
}

float4 basic_blit_fragment(VertexResult vert) : SV_TARGET
{
	return texture.Sample(sampler, vert.texCoords).rgba;
}
