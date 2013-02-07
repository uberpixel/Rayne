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
uniform vec4 lightPosition[20];
uniform vec3 lightColor[20];

in vec2 outTexcoord;
in vec3 outNormal;
in vec3 outPosition;

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, outTexcoord);
	if(color0.a < 0.3)
		discard;
	
	vec3 normal = normalize(outNormal);
	
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0f;
	vec3 light = vec3(0.0);
	for(int i = 0; i < 5; i++)
	{
		posdiff = lightPosition[i].xyz-outPosition;
		attenuation = max((lightPosition[i].w-length(posdiff))/lightPosition[i].w, 0.0);
		light += lightColor[i]*dot(normal, normalize(posdiff))*attenuation*attenuation;
	}
	
	color0.rgb *= light;
	fragColor0 = color0;
}
