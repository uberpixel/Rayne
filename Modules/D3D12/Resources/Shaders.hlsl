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

// Variables in constant address space
static const float3 light_position = float3(-1.0, 1.0, 1.0);

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

cbuffer fragmentUniforms : register(b1)
{
	float4 ambientColor;
	float4 diffuseColor;

#if RN_DISCARD
	float discardThreshold;
#endif
};

/*struct LightDirectional
{
	float3 direction;
	float4 color;
}

cbuffer lightUniforms : register(b2)
{
	uint directionalLightsCount;
	LightDirectional directionalLights[5];
};*/

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

FragmentVertex gouraud_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0));

#if RN_COLOR
	result.color = vert.color;
#endif
#if RN_NORMALS
	result.normal = mul(modelMatrix, float4(vert.normal, 0.0)).xyz;
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
	return color * (ambientColor + saturate(dot(normalize(vert.normal), normalize(light_position))));
#else
	return color * (ambientColor);
#endif
}
