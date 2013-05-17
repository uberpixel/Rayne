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

#ifdef RN_LIGHTING

#define USE_UBOs 0

uniform isamplerBuffer lightPointList;
#if !USE_UBOs
uniform isamplerBuffer lightPointListOffset;
uniform samplerBuffer lightPointListData;
#else
uniform lightPointListOffsetUBO
{
	ivec4 lightPointListOffset[32*32];
};

uniform lightPointListDataUBO
{
	vec4 lightPointData[2048];
};
#endif

uniform isamplerBuffer lightSpotList;
uniform isamplerBuffer lightSpotListOffset;
uniform samplerBuffer lightSpotListData;

uniform int lightDirectionalCount;
uniform vec3 lightDirectionalDirection[10];
uniform vec3 lightDirectionalColor[10];

uniform vec4 lightTileSize;

in vec3 outLightNormal;
in vec3 outLightPosition;

vec3 rn_PointLight(int index, vec3 normal, vec3 position)
{
#if USE_UBOs
	vec4 lightpos   = lightPointData[index];
	vec3 lightcolor = lightPointData[index+1].xyz;
#else
	vec4 lightpos   = texelFetch(lightPointListData, index);
	vec3 lightcolor = texelFetch(lightPointListData, index + 1).xyz;
#endif
	
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	
	return lightcolor*max(dot(normal, posdiff/dist), 0.0)*attenuation*attenuation*2.0;
}

vec3 rn_SpotLight(int index, vec3 normal, vec3 position)
{
	vec4 lightpos   = texelFetch(lightSpotListData, index);
	vec3 lightcolor = texelFetch(lightSpotListData, index + 1).xyz;
	vec4 lightdir   = texelFetch(lightSpotListData, index + 2);
	
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	posdiff /= dist;
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	float dirfac = dot(posdiff, lightdir.xyz);
	
	if(dirfac > lightdir.w)
		return lightcolor*max(dot(normal, posdiff), 0.0)*attenuation*attenuation;
	
	return vec3(0.0);
}

vec3 rn_DirectionalLight(int index, vec3 normal)
{
	return lightDirectionalColor[index]*max(dot(normal, lightDirectionalDirection[index]), 0.0)*2.0;//*rn_ShadowDir1();
}

vec4 rn_Lighting()
{
	vec3 normal = normalize(outLightNormal);
	if(!gl_FrontFacing)
	{
		normal *= -1.0;
	}
	
	vec3 light = vec3(0.0);
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	
#if USE_UBOs
	ivec2 listoffset = lightPointListOffset[tileindex].xy;
#else
	ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
#endif
	int count = listoffset.y;
	for(int i=0; i<count; i++)
	{
		int lightindex = (texelFetch(lightPointList, listoffset.x + i).r) * 2;		
		light += rn_PointLight(lightindex, normal, outLightPosition);
	}
/*
	if(listoffset.y < 5)
		light *= vec3(1.0);
	else if(listoffset.y < 10)
		light *= vec3(0.0, 1.0, 0.0);
	else if(listoffset.y < 15)
		light *= vec3(0.0, 0.0, 1.0);
	else
		light *= vec3(1.0, 0.0, 0.0);
*/
	
	listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightSpotList, listoffset.x + i).r) * 3;
		light += rn_SpotLight(lightindex, normal, outLightPosition);
	}
	
	for(int i=0; i<lightDirectionalCount; i++)
	{
		light += rn_DirectionalLight(i, normal);
	}
	
	return vec4(light, 1.0);
}

#else

#define rn_Lighting() (vec4(1.0))

#endif
#endif
