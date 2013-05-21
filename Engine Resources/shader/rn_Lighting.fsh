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

vec3 rn_PointLight(in vec3 viewdir, in vec4 lightpos, in vec3 lightcolor, in vec3 normal, in vec3 position, out vec3 specularity)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	
#ifdef RN_SPECULARITY
	if(attenuation < 0.0001)
	{
		specularity = vec3(0.0);
		return vec3(0.0);
	}
	
	vec3 halfvec = normalize(viewdir+posdiff);
	specularity = pow(min(max(dot(halfvec, normal), 0.0), 1.0), 32.0)*lightcolor*attenuation;
#endif
	
	return lightcolor*max(dot(normal, posdiff/dist), 0.0)*attenuation*attenuation*2.0;
}

vec3 rn_SpotLight(in vec3 viewdir, in vec4 lightpos, in vec3 lightcolor, in vec4 lightdir, in vec3 normal, in vec3 position, out vec3 specularity)
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
			specularity = vec3(0.0);
			return vec3(0.0);
		}
		
		vec3 halfvec = normalize(viewdir+posdiff);
		specularity = pow(min(max(dot(halfvec, normal), 0.0), 1.0), 32.0)*lightcolor*attenuation;
#endif
		
		return lightcolor*max(dot(normal, posdiff), 0.0)*attenuation*attenuation;
	}
	
	return vec3(0.0);
}

vec3 rn_DirectionalLight(vec3 lightdir, vec4 lightcolor, vec3 normal)
{
	vec3 light = lightcolor.rgb*max(dot(normal, lightdir), 0.0)*2.0;
	
#ifdef RN_DIRECTIONAL_SHADOWS
	if(lightcolor.a > 0.5)
		return light*rn_ShadowDir1();
	else
#endif
		return light;
	
/*#ifdef RN_SPECULARITY
	if(attenuation < 0.0001)
	{
		specularity = vec3(0.0);
		return vec3(0.0);
	}
	
	vec3 halfvec = normalize(viewdir+lightdir);
	specularity = pow(min(max(dot(halfvec, normal), 0.0), 1.0), 32.0)*lightcolor*attenuation;
#endif*/
}

vec3 rn_PointLightTiled(in int index, in vec3 viewdir, in vec3 normal, in vec3 position, out vec3 specularity)
{
	vec4 lightpos   = texelFetch(lightPointListData, index);
	vec3 lightcolor = texelFetch(lightPointListData, index + 1).xyz;
	
	return rn_PointLight(viewdir, lightpos, lightcolor, normal, position, specularity);
}

vec3 rn_SpotLightTiled(int index, vec3 viewdir, vec3 normal, vec3 position, vec3 specularity)
{
	vec4 lightpos   = texelFetch(lightSpotListData, index);
	vec3 lightcolor = texelFetch(lightSpotListData, index + 1).xyz;
	vec4 lightdir   = texelFetch(lightSpotListData, index + 2);
	
	return rn_SpotLight(viewdir, lightpos, lightcolor, lightdir, normal, position, specularity);
}

vec4 rn_Lighting(vec4 color, vec3 specularity, vec3 normal, vec3 position)
{
	if(!gl_FrontFacing)
	{
		normal *= -1.0;
	}
	
	vec3 light = ambient.rgb;
	vec3 specsum = vec3(0.0);
	vec3 viewdir = viewPosition-position;
	
#if !(defined(RN_POINT_LIGHTS_FASTPATH) || defined(RN_SPOT_LIGHTS_FASTPATH))
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
#endif
	
#ifndef RN_POINT_LIGHTS_FASTPATH
	ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightPointList, listoffset.x + i).r) * 2;
		
		vec3 spec;
		light += rn_PointLightTiled(lightindex, viewdir, normal, position, spec);
		specsum += spec;
	}
#else
	for(int i=0; i<RN_POINT_LIGHTS; i++)
	{
		vec3 spec;
		light += rn_PointLight(viewdir, lightPointPosition[i], lightPointColor[i], normal, position, spec);
		specsum += spec;
	}
#endif
	
#ifndef RN_SPOT_LIGHTS_FASTPATH
	listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightSpotList, listoffset.x + i).r) * 3;
		
		vec3 spec;
		light += rn_SpotLightTiled(lightindex, viewdir, normal, position, spec);
		specsum += spec;
	}
#else
	for(int i=0; i<RN_SPOT_LIGHTS; i++)
	{
		vec3 spec;
		light += rn_SpotLight(viewdir, lightSpotPosition[i], lightSpotColor[i], lightSpotDirection[i], normal, position, spec);
		specsum += spec;
	}
#endif
	
	for(int i=0; i<lightDirectionalCount; i++)
	{
		light += rn_DirectionalLight(lightDirectionalDirection[i], lightDirectionalColor[i], normal);
	}
	
#ifdef RN_SPECULARITY
	return vec4(color.rgb*light+specsum*specularity, color.a);
#else
	return vec4(color.rgb*light, color.a);
#endif
}
#endif
