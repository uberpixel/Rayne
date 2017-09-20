//
//  Depth.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Exported defines:
// RN_UV0
// RN_ALPHA

#if RN_UV0
Texture2D texture0 : register(t0);
SamplerState linearRepeatSampler : register(s0);
#endif

cbuffer vertexUniforms : register(b0)
{
#if RN_SKY
	matrix modelViewMatrix;
	matrix projectionMatrix;
#else
	matrix modelViewProjectionMatrix;
#endif

#if RN_UV0
	float textureTileFactor;
#endif
};

#if RN_UV0 && RN_ALPHA
cbuffer FragmentUniforms : register(b1)
{
	float2 alphaToCoverageClamp;
};
#endif

struct InputVertex
{
	float3 position : POSITION;
#if RN_UV0 && RN_ALPHA
	float2 texCoords : TEXCOORD0;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
#if RN_UV0 && RN_ALPHA
	float2 texCoords : TEXCOORD0;
#endif
};

FragmentVertex depth_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_SKY
	float3 rotatedPosition = mul(modelViewMatrix, vert.position);
	result.position = mul(projectionMatrix, float4(rotatedPosition, 1.0)).xyww;
#else
	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0f));
#endif

#if RN_UV0 && RN_ALPHA
	result.texCoords = vert.texCoords*textureTileFactor;
#endif

	return result;
}


float4 depth_fragment(
#if RN_UV0 && RN_ALPHA
	FragmentVertex vert
#endif
	) : SV_TARGET
{
	float4 color = 1.0f;
#if RN_UV0 && RN_ALPHA
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
	color.a = smoothstep(alphaToCoverageClamp.x, alphaToCoverageClamp.y, color.a);
#endif
	return color;
}
