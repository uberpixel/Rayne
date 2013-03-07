//
//  rn_Lightning.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#define RN_LIGHTNING_FSH

#ifdef RN_LIGHTNING

uniform samplerBuffer lightListColor;
uniform samplerBuffer lightListPosition;
uniform isamplerBuffer lightList;
uniform isamplerBuffer lightListOffset;
uniform vec4 lightTileSize;

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
	int lightindex = 0;
	
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	ivec2 listoffset = texelFetch(lightListOffset, tileindex).xy;
	
	for(int i = 0; i < listoffset.y; i++)
	{
		lightindex = texelFetch(lightList, listoffset.x+i).r;
		lightpos = texelFetch(lightListPosition, lightindex);
		lightcolor = texelFetch(lightListColor, lightindex).xyz;
		
		posdiff = lightpos.xyz-outLightPosition;
		attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
		
		light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
	}
	
	return vec4(light, 1.0);
}

#else

#define rn_Lightning() (vec4(1.0))

#endif
