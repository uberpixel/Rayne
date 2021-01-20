//
//  Sky.hlsl
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#pragma permutator RN_UV0
#pragma permutator RN_COLOR

#if RN_UV0
SamplerState linearClampSampler;
Texture2D texture0;
#endif

cbuffer vertexUniforms
{
#if RN_USE_MULTIVIEW
	matrix modelViewMatrix_multiview[6];
	matrix projectionMatrix_multiview[6];
#else
	matrix modelViewMatrix;
	matrix projectionMatrix;
#endif
};

cbuffer fragmentUniforms
{
	float4 cameraAmbientColor;
	float4 diffuseColor;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;

#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif
#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR0;
#endif

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

#if RN_COLOR
	float3 color : COLOR0;
#endif
};

FragmentVertex sky_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_USE_MULTIVIEW
	float3 rotatedPosition = mul(modelViewMatrix_multiview[vert.viewIndex], float4(vert.position, 0.0)).xyz;
	result.position = mul(projectionMatrix_multiview[vert.viewIndex], float4(rotatedPosition, 0.0)).xyww;
#else
	float3 rotatedPosition = mul(modelViewMatrix, float4(vert.position, 0.0)).xyz;
	result.position = mul(projectionMatrix, float4(rotatedPosition, 0.0)).xyww;
#endif

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

#if RN_COLOR
	result.color = vert.color.rgb;
#endif

	return result;
}


float4 sky_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = diffuseColor;

#if RN_UV0
	color *= texture0.Sample(linearClampSampler, vert.texCoords).rgba;
#endif

#if RN_COLOR
	color.rgb *= vert.color;
#endif

	color.rgb *= cameraAmbientColor.rgb;
	return color;
}
