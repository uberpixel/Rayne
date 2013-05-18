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

uniform int lightDirectionalCount;
uniform vec3 lightDirectionalDirection[10];
uniform vec4 lightDirectionalColor[10];

uniform vec4 lightTileSize;
uniform vec4 ambient;

vec3 rn_PointLight(vec4 lightpos, vec3 lightcolor, vec3 normal, vec3 position)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	
	return lightcolor*max(dot(normal, posdiff/dist), 0.0)*attenuation*attenuation*2.0;
}

vec3 rn_SpotLight(vec4 lightpos, vec3 lightcolor, vec4 lightdir, vec3 normal, vec3 position)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	posdiff /= dist;
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	float dirfac = dot(posdiff, lightdir.xyz);
	
	if(dirfac > lightdir.w)
		return lightcolor*max(dot(normal, posdiff), 0.0)*attenuation*attenuation;
	
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
}

vec3 rn_PointLightTiled(int index, vec3 normal, vec3 position)
{
	vec4 lightpos   = texelFetch(lightPointListData, index);
	vec3 lightcolor = texelFetch(lightPointListData, index + 1).xyz;
	
	return rn_PointLight(lightpos, lightcolor, normal, position);
}

vec3 rn_SpotLightTiled(int index, vec3 normal, vec3 position)
{
	vec4 lightpos   = texelFetch(lightSpotListData, index);
	vec3 lightcolor = texelFetch(lightSpotListData, index + 1).xyz;
	vec4 lightdir   = texelFetch(lightSpotListData, index + 2);
	
	return rn_SpotLight(lightpos, lightcolor, lightdir, normal, position);
}

vec4 rn_Lighting(vec4 color, vec3 normal, vec3 position)
{
	if(!gl_FrontFacing)
	{
		normal *= -1.0;
	}
	
	vec3 light = ambient.rgb;
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	
	ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightPointList, listoffset.x + i).r) * 2;		
		light += rn_PointLightTiled(lightindex, normal, position);
	}
	
	listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		int lightindex = (texelFetch(lightSpotList, listoffset.x + i).r) * 3;
		light += rn_SpotLightTiled(lightindex, normal, position);
	}
	
	for(int i=0; i<lightDirectionalCount; i++)
	{
		light += rn_DirectionalLight(lightDirectionalDirection[i], lightDirectionalColor[i], normal);
	}
	
	color.rgb *= light;
	return color;
}
#endif
