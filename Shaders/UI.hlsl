//
//  UI.hlsl
//  Rayne
//
//  Copyright 2021 by SlinDev. All rights reserved.
//

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
#if RN_UI_GRADIENT
	float4 specularColor;
	float4 emissiveColor;
	float4 ambientColor;
#endif

	float4 cameraAmbientColor;

	float4 uiClippingRect;
	float2 uiOffset;

#if RN_UI_OUTLINE
	float4 uiOutlineColor;
#endif
};

struct InputVertex
{
#if RN_UI_OUTLINE
	[[vk::location(0)]] float3 position : POSITION;
#else
	[[vk::location(0)]] float2 position : POSITION;
#endif

#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR;
#endif

#if RN_UV0 || RN_UI_CIRCLE || RN_UI_GRADIENT
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

#if RN_UV0 || RN_UI_CIRCLE || RN_UI_GRADIENT
	float2 texCoords : TEXCOORD2;
#endif
#if RN_UV1
	half3 curveTexCoords : TEXCOORD3;
#endif

#if RN_UI_GRADIENT
	half4 color1 : TEXCOORD4;
	half4 color2 : TEXCOORD5;
	half4 color3 : TEXCOORD6;
#endif
};

FragmentVertex ui_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0 || RN_UI_CIRCLE || RN_UI_GRADIENT
	result.texCoords = vert.texCoords;
#endif

#if RN_UV1
	result.curveTexCoords = vert.curveTexCoords;
#endif

	float2 position = vert.position.xy + uiOffset;

#if RN_USE_MULTIVIEW
	result.position = mul(modelViewProjectionMatrix_multiview[vert.viewIndex], float4(position, 0.0, 1.0));
#else
	result.position = mul(modelViewProjectionMatrix, float4(position, 0.0, 1.0));
#endif

	result.clipDistance.x = position.x - uiClippingRect.x;
	result.clipDistance.y = uiClippingRect.y - position.x;
	result.clipDistance.z = -uiClippingRect.z - position.y;
	result.clipDistance.w = position.y + uiClippingRect.w;

	float4 colorFactor = cameraAmbientColor;

#if RN_COLOR
	colorFactor *= vert.color;
#endif

#if RN_UI_OUTLINE
	result.color = vert.position.z < 0.5? diffuseColor : uiOutlineColor;
	result.color *= colorFactor;
#else
	result.color = diffuseColor * colorFactor;
#endif

#if RN_UI_GRADIENT
	result.color1 = specularColor * colorFactor;
	result.color2 = emissiveColor * colorFactor;
	result.color3 = ambientColor * colorFactor;
#endif

	return result;
}


half4 ui_fragment(FragmentVertex vert) : SV_TARGET
{
#if RN_UI_GRADIENT
	half4 topColor = lerp(vert.color, vert.color1, vert.texCoords.x);
	half4 bottomColor = lerp(vert.color3, vert.color2, vert.texCoords.x);
	half4 color = lerp(topColor, bottomColor, vert.texCoords.y);
#else
	half4 color = vert.color;
#endif

#if RN_UV0 && !RN_UI_SDF
	color *= texture0.Sample(linearClampSampler, vert.texCoords.xy).rgba;
#endif

#if RN_UV1
	float curve = (vert.curveTexCoords.x * vert.curveTexCoords.x - vert.curveTexCoords.y);

	float px = ddx(curve);
	float py = ddy(curve);
	float dist = curve / sqrt(px * px + py * py); //Normalize to pixelsize for anti aliasing

	color.a *= saturate(0.5 - dist * vert.curveTexCoords.z);
#endif

#if RN_UI_SDF && RN_UV0
	float3 msd = texture0.Sample(linearClampSampler, vert.texCoords.xy).rgb;
	float sd = max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b)) - 0.5;
	color.a *= clamp(sd/fwidth(sd) + 0.5, 0.0, 1.0);
#endif

#if RN_UI_CIRCLE
	float2 uv = vert.texCoords.xy * 2.0 - 1.0;
	float2 uvPixelSize;
	uvPixelSize.x = ddx(uv.x);
	uvPixelSize.y = ddy(uv.y);
	float uvFinalPixelSize = sqrt(dot(uvPixelSize, uvPixelSize));

	float d = sqrt(dot(uv, uv));
	color.a *= 1.0 - smoothstep(1.0 - uvFinalPixelSize, 1.0, d);
#endif

	float clipping = min(vert.clipDistance.x, min(vert.clipDistance.y, min(vert.clipDistance.z, vert.clipDistance.w)));
	float clippingddx = ddx(clipping);
	float clippingddy = ddy(clipping);
	float clippingfactor = clipping / sqrt(clippingddx * clippingddx + clippingddy * clippingddy);
	color.a *= saturate(0.5 + clippingfactor);

	return color;
}
