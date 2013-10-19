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

#if defined(RN_POINT_LIGHTS_FASTPATH)
	#if (RN_POINT_LIGHTS_FASTPATH > 0)
		uniform vec4 lightPointPosition[RN_POINT_LIGHTS_FASTPATH];
		uniform vec4 lightPointColor[RN_POINT_LIGHTS_FASTPATH];
	#endif
#endif

#if defined(RN_SPOT_LIGHTS_FASTPATH)
	#if (RN_SPOT_LIGHTS_FASTPATH > 0)
		uniform vec4 lightSpotPosition[RN_SPOT_LIGHTS_FASTPATH];
		uniform vec4 lightSpotDirection[RN_SPOT_LIGHTS_FASTPATH];
		uniform vec4 lightSpotColor[RN_SPOT_LIGHTS_FASTPATH];
	#endif
#endif

#if defined(RN_DIRECTIONAL_LIGHTS)
	uniform int lightDirectionalCount;
	uniform vec3 lightDirectionalDirection[RN_DIRECTIONAL_LIGHTS];
	uniform vec4 lightDirectionalColor[RN_DIRECTIONAL_LIGHTS];
#endif

uniform vec3 viewPosition;

uniform vec4 lightTileSize;
uniform vec4 ambient;

void rn_PointLight(in vec3 viewdir, in vec4 lightpos, in vec4 lightcolor, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	vec3 dir = posdiff/dist;
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	
	if(attenuation < 0.0001)
	{
		return;
	}
	
	float lightfac = min(max(dot(normal, dir), 0.0), 1.0);
	vec3 light = lightfac*lightcolor.rgb*attenuation;
	
#if defined(RN_SPECULARITY)
	vec3 halfvec = normalize(viewdir+dir);
	vec3 spec = pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*light;
#endif
	
	light *= attenuation;
	
#if defined(RN_POINT_SHADOWS)
	if(lightcolor.a > -0.5)
	{
		float shadow = rn_ShadowPoint(int(lightcolor.a+0.1), posdiff, lightpos.w);
#if defined(RN_SPECULARITY)
		specularity += spec*shadow;
#endif
		lighting += light*shadow;
	}
	else
#endif
	{
#if defined(RN_SPECULARITY)
		specularity += spec;
#endif
		lighting += light;
	}
}

void rn_SpotLight(in vec3 viewdir, in vec4 lightpos, in vec4 lightcolor, in vec4 lightdir, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec3 posdiff = lightpos.xyz-position;
	float dist = length(posdiff);
	vec3 dir = posdiff/dist;
	float attenuation = min(max(1.0-dist/lightpos.w, 0.0), 1.0);
	float dirfac = dot(dir, lightdir.xyz);
	
	if(dirfac > lightdir.w)
	{
		if(attenuation < 0.0001)
		{
			return;
		}
		
		attenuation *= 1.0-(1.0-dirfac)/(1.0-lightdir.w);
		
		float lightfac = min(max(dot(normal, dir), 0.0), 1.0);
		vec3 light = lightfac*lightcolor.rgb*attenuation;
		
#if defined(RN_SPECULARITY)
		vec3 halfvec = normalize(viewdir+dir);
		vec3 spec = pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*light;
#endif
		light *= attenuation;
		
#if defined(RN_SPOT_SHADOWS)
		if(lightcolor.a > -0.5)
		{
			float shadow = rn_ShadowSpot(int(lightcolor.a+0.1), posdiff, lightpos.w);
#if defined(RN_SPECULARITY)
			specularity += spec*shadow;
#endif
			lighting += light*shadow;
		}
		else
#endif
		{
#if defined(RN_SPECULARITY)
			specularity += spec;
#endif
			lighting += light;
		}
	}
}

void rn_DirectionalLight(in vec3 viewdir, in vec3 lightdir, in vec4 lightcolor, in vec3 normal, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	float lightfac = min(max(dot(normal, lightdir), 0.0), 1.0);
	vec3 light = lightcolor.rgb*lightfac;
	
#if defined(RN_SPECULARITY)
	vec3 halfvec = normalize(viewdir+lightdir);
	vec3 spec = pow(min(max(dot(halfvec, normal), 0.0), 1.0), specpow)*light;
#endif
	
#if defined(RN_DIRECTIONAL_SHADOWS)
	if(lightcolor.a > -0.5)
	{
		float shadow = rn_ShadowDirectional0();
		#if defined(RN_SPECULARITY)
			specularity += spec*shadow;
		#endif
		lighting += light*shadow;
	}
	else
#endif
	{
		#if defined(RN_SPECULARITY)
			specularity += spec;
		#endif
		lighting += light;
	}
}

void rn_PointLightTiled(in int index, in vec3 viewdir, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec4 lightpos   = texelFetch(lightPointListData, index);
	vec4 lightcolor = texelFetch(lightPointListData, index + 1);
	
	rn_PointLight(viewdir, lightpos, lightcolor, normal, position, specpow, lighting, specularity);
}

void rn_SpotLightTiled(in int index, in vec3 viewdir, in vec3 normal, in vec3 position, in float specpow, inout vec3 lighting, inout vec3 specularity)
{
	vec4 lightpos   = texelFetch(lightSpotListData, index);
	vec4 lightcolor = texelFetch(lightSpotListData, index + 1);
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
	
#if (!defined(RN_POINT_LIGHTS_FASTPATH) || !defined(RN_SPOT_LIGHTS_FASTPATH))
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
#endif
	
#if !defined(RN_POINT_LIGHTS_FASTPATH)
	{
		ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
		for(int i=0; i<listoffset.y; i++)
		{
			int lightindex = (texelFetch(lightPointList, listoffset.x + i).r) * 2;
			rn_PointLightTiled(lightindex, viewdir, normal, position, specularity.a, light, specsum);
		}
	}
#elif (RN_POINT_LIGHTS_FASTPATH > 0)
	for(int i=0; i<RN_POINT_LIGHTS_FASTPATH; i++)
	{
		rn_PointLight(viewdir, lightPointPosition[i], lightPointColor[i], normal, position, specularity.a, light, specsum);
	}
#endif
	
#if !defined(RN_SPOT_LIGHTS_FASTPATH)
	{
		ivec2 listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
		for(int i=0; i<listoffset.y; i++)
		{
			int lightindex = (texelFetch(lightSpotList, listoffset.x + i).r) * 3;
			rn_SpotLightTiled(lightindex, viewdir, normal, position, specularity.a, light, specsum);
		}
	}
#elif (RN_SPOT_LIGHTS_FASTPATH > 0)
	for(int i=0; i<RN_SPOT_LIGHTS_FASTPATH; i++)
	{
		rn_SpotLight(viewdir, lightSpotPosition[i], lightSpotColor[i], lightSpotDirection[i], normal, position, specularity.a, light, specsum);
	}
#endif
	
#if defined(RN_DIRECTIONAL_LIGHTS)
	for(int i=0; i<RN_DIRECTIONAL_LIGHTS; i++)
	{
		rn_DirectionalLight(viewdir, lightDirectionalDirection[i], lightDirectionalColor[i], normal, specularity.a, light, specsum);
	}
#endif

#if defined(RN_SPECULARITY)
	color.rgb = color.rgb*light+specsum*specularity.rgb*(specularity.a+1.0)/(2.0*3.1514);
#else
	color.rgb = color.rgb*light;
#endif
}
#endif
