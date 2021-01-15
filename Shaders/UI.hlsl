//
//  UI.hlsl
//  Rayne
//
//  Copyright 2021 by SlinDev. All rights reserved.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

cbuffer vertexUniforms
{
	matrix modelViewProjectionMatrix;
	float4 diffuseColor;
	float4 emissiveColor;
};

struct InputVertex
{
	[[vk::location(0)]] float2 position : POSITION;

#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR;
#endif

#if RN_UV0
	[[vk::location(5)]] float3 texCoords : TEXCOORD0;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD0;
	half4 clipDistance : TEXCOORD1;

#if RN_UV0
	half3 texCoords : TEXCOORD2;
#endif
};

FragmentVertex ui_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 0.0, 1.0));

	result.clipDistance.x = vert.position.x - emissiveColor.x;
	result.clipDistance.y = emissiveColor.y - vert.position.x;
	result.clipDistance.z = -emissiveColor.z - vert.position.y;
	result.clipDistance.w = vert.position.y + emissiveColor.w;

#if RN_COLOR
	result.color = vert.color * diffuseColor;
#else
	result.color = diffuseColor;
#endif

	return result;
}


half4 ui_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

#if RN_UV0
	float curve = (vert.texCoords.x * vert.texCoords.x - vert.texCoords.y);

	float px = ddx(curve);
	float py = ddy(curve);
	float dist = curve / sqrt(px * px + py * py); //Normalize to pixelsize for anti aliasing

	color.a *= saturate(0.5 - dist * vert.texCoords.z);
#endif

	float blubb = min(vert.clipDistance.x, min(vert.clipDistance.y, min(vert.clipDistance.z, vert.clipDistance.w)));
	float blubbpx = ddx(blubb);
	float blubbpy = ddy(blubb);
	float haha = blubb / sqrt(blubbpx * blubbpx + blubbpy * blubbpy);
	color.a *= saturate(0.5 + haha);

	return color;
}
