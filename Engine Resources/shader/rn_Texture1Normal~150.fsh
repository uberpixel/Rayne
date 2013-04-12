//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Discard.fsh"

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;

in vec2 outTexcoord;
in mat3 outTangentToWorldMatrix;
out vec4 fragColor0;

uniform isamplerBuffer lightPointList;
uniform isamplerBuffer lightPointListOffset;
uniform samplerBuffer lightPointListData;

uniform int lightDirectionalCount;
uniform vec3 lightDirectionalDirection[10];
uniform vec3 lightDirectionalColor[10];
uniform sampler2DArrayShadow lightDirectionalDepth;

uniform vec2 clipPlanes;
uniform vec4 frameSize;

uniform vec4 lightTileSize;
uniform vec3 viewPosition;

in vec3 outLightPosition;
in vec4 outDirLightProj[4];

float offset_lookup(sampler2DArrayShadow map, vec4 loc, vec2 offset)
{
	return texture(map, vec4(loc.xy+offset*frameSize.xy, loc.wz));
}

vec4 rn_Lighting(vec3 normal, vec4 color, float specmask)
{
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0;
	vec3 light = vec3(0.0);
	vec4 lightpos;
	vec3 lightcolor;
	vec3 lightspec = vec3(0.0);
	float spec;
	float lightdot;
	vec3 halfvec;
	int lightindex = 0;
	vec3 viewdir = viewPosition-outLightPosition;
	
/*	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	ivec2 listoffset = texelFetch(lightPointListOffset, tileindex).xy;
	
	for(int i=0; i<listoffset.y; i++)
	{
		lightindex = (texelFetch(lightPointList, listoffset.x+i).r) * 2;
		lightpos   = texelFetch(lightPointListData, lightindex);
		lightcolor = texelFetch(lightPointListData, lightindex + 1).xyz;
		
		posdiff = lightpos.xyz-outLightPosition;
		attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
		attenuation *= attenuation;

		lightdot = max(dot(normal, normalize(posdiff)), 0.0);
		light += lightcolor*lightdot*attenuation;
		halfvec = normalize(viewdir+normalize(posdiff));
		
		spec = max(dot(normal, halfvec), 0.0)*lightdot;
		spec = pow(spec, 16.0);
		lightspec += lightcolor*spec*attenuation;
	}*/
	
	for(int i=0; i<lightDirectionalCount; i++)
	{
		float comp = (clipPlanes.x * clipPlanes.y)/(clipPlanes.y-gl_FragCoord.z*(clipPlanes.y-clipPlanes.x));
		
		vec4  zGreater = vec4(lessThan(vec4(15.0, 30.0, 60.0, 150.0), vec4(comp)));
		float   mapToUse = dot(zGreater, vec4(1.0f));
		vec4 projected = vec4(outDirLightProj[int(mapToUse)].xyz/outDirLightProj[int(mapToUse)].w, mapToUse);
		
		float shadow = offset_lookup(lightDirectionalDepth, projected, vec2(-1.0, 0.0));
		shadow += offset_lookup(lightDirectionalDepth, projected, vec2(1.0, 0.0));
		shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, -1.0));
		shadow += offset_lookup(lightDirectionalDepth, projected, vec2(0.0, 1.0));
		
		shadow *= 0.25;
		
		lightdot = max(dot(normal, lightDirectionalDirection[i]), 0.0);
		light += lightDirectionalColor[i]*lightdot*shadow;
		
		halfvec = normalize(normalize(viewdir)+lightDirectionalDirection[i]);
		spec = max(dot(normal, halfvec), 0.0)*lightdot;
		spec = pow(spec, 16.0);
		lightspec += lightDirectionalColor[i]*spec;
	}
	
	return color*vec4(light, 1.0)+vec4(lightspec, 0.0);

//	return color;//*vec4(light, 1.0)+vec4(lightspec, 0.0)*specmask;
}

void main()
{
	vec4 color0 = texture(mTexture0, outTexcoord);
	vec4 norm0 = texture(mTexture1, outTexcoord);
	vec3 normal = norm0.xyz*2.0-1.0;
	normal = normalize(outTangentToWorldMatrix*normal);

	rn_Discard(color0);
	
	fragColor0 = rn_Lighting(normal, color0, norm0.a);
}
