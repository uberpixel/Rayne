//
//  Shaders.hlsl
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

// Exported defines:
// RN_NORMALS
// RN_COLOR
// RN_UV0
// RN_DISCARD

#if RN_UV0
Texture2D texture0 : register(t0);
SamplerState samplr : register(s0);
#endif

cbuffer vertexUniforms : register(b0)
{
	matrix modelViewProjectionMatrix;
	matrix modelMatrix;

#if RN_UV0
	float textureTileFactor;
#endif
};

struct LightDirectional
{
	float4 direction;
	float4 color;
};

cbuffer fragmentUniforms : register(b1)
{
	float4 ambientColor;
	float4 diffuseColor;

#if RN_DISCARD
	float discardThreshold;
#endif

	uint directionalLightsCount;
	LightDirectional directionalLights[5];
};

struct InputVertex
{
	float3 position : POSITION;

#if RN_NORMALS
	float3 normal : NORMAL;
#endif
#if RN_COLOR
	float4 color : COLOR0;
#endif
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
#if RN_TANGENTS
	float4 tangents : TANGENT;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;

#if RN_NORMALS
	float3 normal : NORMAL;
#endif
#if RN_COLOR
	float4 color : COLOR0;
#endif
#if RN_UV0
	float2 texCoords : TEXCOORD0;
#endif
};

float4 getDirectionalLights(float3 normal, uint count, LightDirectional directionalLights[5])
{
	float4 light = 0.0f;
	for(uint i = 0; i < count; i++)
	{
		light += saturate(dot(normal, directionalLights[i].direction.xyz)) * directionalLights[i].color;
	}

	return light;
}

FragmentVertex gouraud_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0f));

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS
	result.normal = mul(modelMatrix, float4(vert.normal, 0.0f)).xyz;
#endif
#if RN_UV0
	result.texCoords = vert.texCoords*textureTileFactor;
#endif

	return result;
}


float4 gouraud_fragment(FragmentVertex vert) : SV_TARGET
{
	float4 color = diffuseColor;
#if RN_UV0
	color *= texture0.Sample(samplr, vert.texCoords).rgba;

#if RN_DISCARD
	clip(color.a - discardThreshold);
#endif
#endif

#if RN_COLOR
	color *= vert.color;
#endif

#if RN_NORMALS
	float4 light = getDirectionalLights(normalize(vert.normal), directionalLightsCount, directionalLights);
	return color * (ambientColor + light);
#else
	return color * (ambientColor);
#endif
}
