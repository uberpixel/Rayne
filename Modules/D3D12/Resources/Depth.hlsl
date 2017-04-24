//
//  Depth.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Exported defines:
// RN_UV0
// RN_DISCARD

#if RN_UV0
Texture2D texture0 : register(t0);
SamplerState linearRepeatSampler : register(s0);
#endif

cbuffer vertexUniforms : register(b0)
{
	matrix modelViewProjectionMatrix;

#if RN_UV0
	float textureTileFactor;
#endif
};

#if RN_UV0
cbuffer fragmentUniforms : register(b1)
{
#if RN_DISCARD
	float discardThreshold;
#endif
};
#endif

struct InputVertex
{
	float3 position : POSITION;
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
};

FragmentVertex depth_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0f));

#if RN_UV0
	result.texCoords = vert.texCoords*textureTileFactor;
#endif

	return result;
}


float4 depth_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = 1.0f;
#if RN_UV0
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;

#if RN_DISCARD
	clip(color.a - discardThreshold);
#endif
#endif

	return color;
}
