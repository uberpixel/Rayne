//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;

uniform samplerBuffer lightListPosition;
uniform isamplerBuffer lightList;
uniform isamplerBuffer lightListIndex;

uniform vec3 lightColor[50];

in vec2 outTexcoord;
in vec3 outNormal;
in vec3 outPosition;

out vec4 fragColor0;

void main()
{
	vec3 normal = normalize(outNormal);
	
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0;
	vec3 light = vec3(0.0);
	vec4 lightpos;
	int lightindex = 0;
	int tileindex = int((gl_FragCoord.x-0.5)/64.0)*24+int((gl_FragCoord.y-0.5)/64.0);
	int listoffset = texelFetch(lightListIndex, tileindex*2).r;
	int lightcount = texelFetch(lightListIndex, tileindex*2+1).r;
	for(int i = 0; i < lightcount; i++)
	{
		lightindex = texelFetch(lightList, listoffset+i).r;
		lightpos = texelFetch(lightListPosition, lightindex);
		posdiff = lightpos.xyz-outPosition;
		attenuation = lightpos.w-length(posdiff);
		attenuation = max(attenuation/lightpos.w, 0.0);
/*		if(attenuation < 0.0)
			attenuation = 0.0;
		else
			attenuation = 0.3;*/
		light += lightColor[i]*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
	}
	
	vec4 color0 = texture(mTexture0, outTexcoord);
	color0.rgb *= light+0.2;
	fragColor0 = color0;//vec4((lightcount)/30.0);
}
