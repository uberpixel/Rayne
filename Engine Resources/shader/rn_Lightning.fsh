//
//  rn_Lightning.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#define RN_LIGHTNING_FSH

#ifdef RN_LIGHTNING

uniform samplerBuffer lightPointListColor;
uniform samplerBuffer lightPointListPosition;
uniform isamplerBuffer lightPointList;
uniform isamplerBuffer lightPointListOffset;

uniform samplerBuffer lightSpotListColor;
uniform samplerBuffer lightSpotListPosition;
uniform samplerBuffer lightSpotListDirection;
uniform isamplerBuffer lightSpotList;
uniform isamplerBuffer lightSpotListOffset;

uniform vec4 lightTileSize;
uniform int lightPointCount;
uniform int lightSpotCount;

in vec3 outLightNormal;
in vec3 outLightPosition;

vec4 rn_Lightning()
{
	vec3 normal = normalize(outLightNormal);
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0;
	vec3 light = vec3(0.0);
	vec4 lightpos;
	vec3 lightcolor;
	vec4 lightdir;
	float dirfac;
	int lightindex = 0;
	
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	
	//pointlights
	if(lightPointCount > 0)
	{
		ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
		for(int i = 0; i < listoffset.y; i++)
		{
			lightindex = texelFetch(lightPointList, listoffset.x+i).r;
			lightpos = texelFetch(lightPointListPosition, lightindex);
			lightcolor = texelFetch(lightPointListColor, lightindex).xyz;
			
			posdiff = lightpos.xyz-outLightPosition;
			attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
			
			light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
		}
	}
	
	//spotlights
	if(lightSpotCount > 0)
	{
		ivec2 listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
		for(int i = 0; i < listoffset.y; i++)
		{
			lightindex = texelFetch(lightSpotList, listoffset.x+i).r;
			lightpos = texelFetch(lightSpotListPosition, lightindex);
			lightcolor = texelFetch(lightSpotListColor, lightindex).xyz;
			lightdir = texelFetch(lightSpotListDirection, lightindex);
			
			posdiff = lightpos.xyz-outLightPosition;
			attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
			dirfac = dot(normalize(posdiff), -lightdir.xyz);
			
			if(dirfac > lightdir.w)
				light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
		}
	}
	
	return vec4(light, 1.0);
}

#else

#define rn_Lightning() (vec4(1.0))

#endif
