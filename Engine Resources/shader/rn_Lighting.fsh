//
//  rn_Lighting.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#define RN_LIGHTING_FSH

#ifdef RN_LIGHTING

uniform isamplerBuffer lightPointList;
uniform isamplerBuffer lightPointListOffset;
uniform samplerBuffer lightPointListData;

uniform isamplerBuffer lightSpotList;
uniform isamplerBuffer lightSpotListOffset;
uniform samplerBuffer lightSpotListData;

uniform int lightDirectionalCount;
uniform vec3 lightDirectionalDirection[10];
uniform vec3 lightDirectionalColor[10];
uniform sampler2DArrayShadow lightDirectionalDepth;

uniform vec2 clipPlanes;
uniform vec4 frameSize;

uniform vec4 lightTileSize;
in vec3 outLightNormal;
in vec3 outLightPosition;
in vec4 outDirLightProj[4];

float offset_lookup(sampler2DArrayShadow map, vec4 loc, vec2 offset)
{
	return texture(map, vec4(loc.xy+offset*frameSize.xy, loc.w, loc.z));
}

float random(vec4 seed4)
{
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

vec4 rn_Lighting()
{
	vec3 normal = normalize(outLightNormal);
	if(!gl_FrontFacing)
	{
		normal *= -1.0;
	}
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0;
	vec3 light = vec3(0.4);
	vec4 lightpos;
	vec3 lightcolor;
	vec4 lightdir;
	float dirfac;
	int lightindex;
	ivec2 listoffset;
	
/*	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	
	listoffset = texelFetch(lightPointListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		lightindex = (texelFetch(lightPointList, listoffset.x + i).r) * 2;

		lightpos   = texelFetch(lightPointListData, lightindex);
		lightcolor = texelFetch(lightPointListData, lightindex + 1).xyz;
		
		posdiff     = lightpos.xyz-outLightPosition;
		attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
		
		light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
	}

	listoffset = texelFetch(lightSpotListOffset, tileindex).xy;
	for(int i=0; i<listoffset.y; i++)
	{
		lightindex = (texelFetch(lightSpotList, listoffset.x + i).r) * 3;

		lightpos   = texelFetch(lightSpotListData, lightindex);
		lightcolor = texelFetch(lightSpotListData, lightindex + 1).xyz;
		lightdir   = texelFetch(lightSpotListData, lightindex + 2);
		
		posdiff = lightpos.xyz-outLightPosition;
		attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
		dirfac = dot(normalize(posdiff), lightdir.xyz);
		
		if(dirfac > lightdir.w)
			light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
	}*/
	
//	for(int i=0; i<lightDirectionalCount; i++)
//	{
		vec3 proj[4];
		proj[0] = outDirLightProj[0].xyz/outDirLightProj[0].w;
		proj[1] = outDirLightProj[1].xyz/outDirLightProj[1].w;
		proj[2] = outDirLightProj[2].xyz/outDirLightProj[2].w;
		proj[3] = outDirLightProj[3].xyz/outDirLightProj[3].w;
		
		vec4 dist = vec4(dot(proj[0], proj[0]), dot(proj[1], proj[1]), dot(proj[2], proj[2]), dot(proj[3], proj[3]));

		vec4 zGreater = vec4(greaterThan(dist, vec4(1.0)));
		float mapToUse = dot(zGreater, vec4(1.0f));
		
/*		float shadow = 1.0;
		if(mapToUse < 4.0)
		{*/
			vec4 projected = vec4(proj[int(mapToUse)]*0.5+0.5, mapToUse);
			
/*			vec2 poissonDisk[4] = vec2[](
			vec2( -0.94201624, -0.39906216 ),
			vec2( 0.94558609, -0.76890725 ),
			vec2( -0.094184101, -0.92938870 ),
			vec2( 0.34495938, 0.29387760 )
			);
			shadow = 0.0;
			for(int i = 0; i < 4; i++)
			{
				int index = int(16.0*random(vec4(gl_FragCoord.xyy, float(i))))%4;
				shadow += offset_lookup(lightDirectionalDepth, projected, poissonDisk[index]*10.0);
			}
			shadow *= 0.25;*/
			
/*			shadow = 0.0;
			for(float offx = -2.0; offx <= 1.0; offx += 1.0)
			{
				for(float offy = -2.0; offy <= 1.0; offy += 1.0)
				{
					shadow += offset_lookup(lightDirectionalDepth, projected, vec2(offx, offy));
				}
			}*/
			
/*			shadow = offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, -1.0)*5.0);
			shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, -1.0)*5.0);
			shadow += offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, 1.0)*5.0);
			shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, 1.0)*5.0);
			
			shadow *= 0.25;*/
/*			if(shadow > 0.1 || shadow < 3.9)
			{
				shadow = offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, -1.0));
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, -1.0));
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, -1.0));
			
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, 0.0));
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, 0.0));
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, 0.0));
			
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, 1.0));
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, 1.0));
				shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, 1.0));
			
				shadow /= 9.0;
			}*/

//			shadow = offset_lookup(lightDirectionalDepth, projected, vec2(0.0, 0.0));
			float shadow = offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, 0.0));
			shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, 0.0));
			shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, -1.0));
			shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, 1.0));
			shadow *= 0.25;
//		}

		light += lightDirectionalColor[0]*max(dot(normal, lightDirectionalDirection[0]), 0.0)*shadow;
//	}
	
	return vec4(light, 1.0);
}

#else

#define rn_Lighting() (vec4(1.0))

#endif
