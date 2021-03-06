//
//  UI.hlsl
//  Rayne
//
//  Copyright 2021 by SlinDev. All rights reserved.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_UV1
#define RN_UV1 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#if RN_UV0
Texture2D texture0;
SamplerState linearClampSampler;
#endif

cbuffer vertexUniforms
{
#if RN_USE_MULTIVIEW
	matrix modelViewProjectionMatrix_multiview[6];
#else
	matrix modelViewProjectionMatrix;
#endif

	float4 diffuseColor;
	float4 cameraAmbientColor;

	float4 uiClippingRect;
	float2 uiOffset;
};

struct InputVertex
{
	[[vk::location(0)]] float2 position : POSITION;

#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR;
#endif

#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif

#if RN_UV1
	[[vk::location(6)]] float3 curveTexCoords : TEXCOORD1;
#endif

#if RN_USE_MULTIVIEW
	uint viewIndex : SV_VIEWID;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD0;
	half4 clipDistance : TEXCOORD1;

#if RN_UV0
	half2 texCoords : TEXCOORD2;
#endif
#if RN_UV1
	half3 curveTexCoords : TEXCOORD3;
#endif
};

FragmentVertex ui_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

#if RN_UV1
	result.curveTexCoords = vert.curveTexCoords;
#endif

	float2 position = vert.position + uiOffset;

#if RN_USE_MULTIVIEW
	result.position = mul(modelViewProjectionMatrix_multiview[vert.viewIndex], float4(position, 0.0, 1.0));
#else
	result.position = mul(modelViewProjectionMatrix, float4(position, 0.0, 1.0));
#endif

	result.clipDistance.x = position.x - uiClippingRect.x;
	result.clipDistance.y = uiClippingRect.y - position.x;
	result.clipDistance.z = -uiClippingRect.z - position.y;
	result.clipDistance.w = position.y + uiClippingRect.w;

#if RN_COLOR
	result.color = vert.color * diffuseColor * cameraAmbientColor;
#else
	result.color = diffuseColor * cameraAmbientColor;
#endif

	return result;
}


half4 ui_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

#if RN_UV0
	color *= texture0.Sample(linearClampSampler, vert.texCoords).rgba;
#endif

#if RN_UV1
	float curve = (vert.curveTexCoords.x * vert.curveTexCoords.x - vert.curveTexCoords.y);

	float px = ddx(curve);
	float py = ddy(curve);
	float dist = curve / sqrt(px * px + py * py); //Normalize to pixelsize for anti aliasing

	color.a *= saturate(0.5 - dist * vert.curveTexCoords.z);
#endif

	float clipping = min(vert.clipDistance.x, min(vert.clipDistance.y, min(vert.clipDistance.z, vert.clipDistance.w)));
	float clippingddx = ddx(clipping);
	float clippingddy = ddy(clipping);
	float clippingfactor = clipping / sqrt(clippingddx * clippingddx + clippingddy * clippingddy);
	color.a *= saturate(0.5 + clippingfactor);

	return color;
}
