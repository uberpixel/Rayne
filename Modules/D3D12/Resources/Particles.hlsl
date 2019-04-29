//
//  Particles.hlsl
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


//Texture2D texture0 : register(t0);
//SamplerState linearRepeatSampler : register(s0);

cbuffer vertexUniforms : register(b0)
{
	matrix inverseViewMatrix;
	matrix modelViewProjectionMatrix;
};

struct InputVertex
{
	float3 position : POSITION;
	float2 texCoords : TEXCOORD0;
	float2 texCoords2 : TEXCOORD1;
	float4 color : COLOR;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
//	float2 texCoords : TEXCOORD0;
	float4 color : COLOR;
};


FragmentVertex particles_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0) + mul(inverseViewMatrix, float4(vert.texCoords2, 0.0, 0.0)));
	result.color = vert.color;

//	result.texCoords = vert.texCoords;

	return result;
}


float4 particles_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = vert.color;//texture0.Sample(linearRepeatSampler, vert.texCoords).rgba * diffuseColor * cameraAmbientColor * light;
	color.rgb *= color.a;

	return color;
}
