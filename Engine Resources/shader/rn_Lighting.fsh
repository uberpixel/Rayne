//
//  rn_Lighting.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_LIGHTING_FSH
#define RN_LIGHTING_FSH

#include "rn_Shadow.fsh"

uniform isamplerBuffer lightPointList;
uniform isamplerBuffer lightPointListOffset;
uniform samplerBuffer lightPointListData;

uniform isamplerBuffer lightSpotList;
uniform isamplerBuffer lightSpotListOffset;
uniform samplerBuffer lightSpotListData;

uniform vec4 lightPointPosition[10];
uniform vec3 lightPointColor[10];

uniform vec4 lightSpotPosition[10];
uniform vec4 lightSpotDirection[10];
uniform vec3 lightSpotColor[10];

uniform int lightDirectionalCount;
uniform vec3 lightDirectionalDirection[10];
uniform vec4 lightDirectionalColor[10];

uniform vec3 viewPosition;

uniform vec4 lightTileSize;
uniform vec4 ambient;

void rn_PointLight(in vec3 viewdir, in vec4 lightpos, in vec3 lightcolor, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	posdiff /= dist;
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	
#ifdef RN_SPECULARITY
	if(attenuation < 0.0001)
	{
		return;
	}
	
	vec3 halfvec = normalize(viewdir+posdiff);
	specularity += pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*lightcolor*attenuation;
#endif
	
	lighting += lightcolor*max(dot(normal, posdiff), 0.0)*attenuation*attenuation*2.0;
}

void rn_SpotLight(in vec3 viewdir, in vec4 lightpos, in vec3 lightcolor, in vec4 lightdir, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	posdiff /= dist;
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	float dirfac = dot(posdiff, lightdir.xyz);
	
	if(dirfac > lightdir.w)
	{
#ifdef RN_SPECULARITY
		if(attenuation < 0.0001)
		{
			return;
		}
		
		vec3 halfvec = normalize(viewdir+posdiff);
		specularity += pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*lightcolor*attenuation;
#endif
		
		lighting += lightcolor*max(dot(normal, posdiff), 0.0)*attenuation*attenuation;
	}
}

void rn_DirectionalLight(in vec3 viewdir, in vec3 lightdir, in vec4 lightcolor, in vec3 normal, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
#ifdef RN_DIRECTIONAL_SHADOWS
	if(lightcolor.a > 0.5)
	{
		float shadow = rn_ShadowDir1();
		
		if(shadow > 0.0001)
		{
			vec3 light = lightcolor.rgb*max(dot(normal, lightdir), 0.0)*2.0;
			
#ifdef RN_SPECULARITY
			vec3 halfvec = normalize(viewdir+lightdir);
			specularity += pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*lightcolor.rgb*shadow;
#endif
			
			lighting += light*shadow;
		}
	}
	else
#endif
	{
		vec3 light = lightcolor.rgb*max(dot(normal, lightdir), 0.0)*2.0;
		
#ifdef RN_SPECULARITY
		vec3 halfvec = normalize(viewdir+lightdir);
		specularity += pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*lightcolor.rgb;
#endif
		
		lighting += light;
	}
}

void rn_PointLightTiled(in int index, in vec3 viewdir, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec4 lightpos   = texelFetch(lightPointListData, index);
	vec3 lightcolor = texelFetch(lightPointListData, index + 1).xyz;
	
	rn_PointLight(viewdir, lightpos, lightcolor, normal, position, specpow, lighting, specularity);
}

void rn_SpotLightTiled(in int index, in vec3 viewdir, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec4 lightpos   = texelFetch(lightSpotListData, index);
	vec3 lightcolor = texelFetch(lightSpotListData, index + 1).xyz;
	vec4 lightdir   = texelFetch(lightSpotListData, index + 2);
	
	rn_SpotLight(viewdir, lightpos, lightcolor, lightdir, normal, position, specpow, lighting, specularity);
}

void rn_Lighting(inout vec4 color, in vec4 specularity, in vec3 normal, in vec3 position)
{
	if(!gl_FrontFacing)
	{
		normal *= -1.0;
	}
	
	vec3 light = ambient.rgb;
	vec3 specsum = vec3(0.0);
	vec3 viewdir = normalize(viewPosition-position);
	
#if !(defined(RN_POINT_LIGHTS_FASTPATH) || defined(RN_SPOT_LIGHTS_FASTPATH))
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
#endif
	
#ifndef RN_POINT_LIGHTS_FASTPATH
	ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightPointList, listoffset.x + i).r) * 2;
		rn_PointLightTiled(lightindex, viewdir, normal, position, specularity.a, light, specsum);
	}
#else
	for(int i=0; i<RN_POINT_LIGHTS; i++)
	{
		rn_PointLight(viewdir, lightPointPosition[i], lightPointColor[i], normal, position, specularity.a, light, specsum);
	}
#endif
	
#ifndef RN_SPOT_LIGHTS_FASTPATH
	listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightSpotList, listoffset.x + i).r) * 3;
		rn_SpotLightTiled(lightindex, viewdir, normal, position, specularity.a, light, specsum);
	}
#else
	for(int i=0; i<RN_SPOT_LIGHTS; i++)
	{
		rn_SpotLight(viewdir, lightSpotPosition[i], lightSpotColor[i], lightSpotDirection[i], normal, position, specularity.a, light, specsum);
	}
#endif
	
	for(int i=0; i<lightDirectionalCount; i++)
	{
		rn_DirectionalLight(viewdir, lightDirectionalDirection[i], lightDirectionalColor[i], normal, specularity.a, light, specsum);
	}
	
#ifdef RN_SPECULARITY
	color.rgb = color.rgb*light+specsum*specularity.rgb;
#else
	color.rgb = color.rgb*light;
#endif
}
#endif
