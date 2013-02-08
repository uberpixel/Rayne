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
uniform vec4 lightPosition[50];
uniform vec3 lightColor[50];
uniform int lightCount;

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
	for(int i = 0; i < lightCount; i++)
	{
		posdiff = lightPosition[i].xyz-outPosition;
		attenuation = lightPosition[i].w-length(posdiff);
//		if(attenuation < 0.0)
//			continue;
//		attenuation = max(attenuation/lightPosition[i].w, 0.0);
		if(attenuation < 0.0)
			attenuation = 0.0;
		else
			attenuation = 0.3;
		light += lightColor[i]/**max(dot(normal, normalize(posdiff)), 0.0)*/*attenuation;//*attenuation;
	}
	
	vec4 color0 = texture(mTexture0, outTexcoord);
	color0.rgb *= light;//+0.2;
	fragColor0 = color0;
}
