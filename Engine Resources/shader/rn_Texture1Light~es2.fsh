//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform sampler2D mTexture0;
uniform vec4 lightPosition[20];
uniform vec3 lightColor[20];
uniform int lightCount;

varying vec2 outTexcoord;
varying vec3 outNormal;
varying vec3 outPosition;

void main()
{
	mediump vec3 normal = normalize(outNormal);
	
	mediump vec3 posdiff = vec3(0.0);
	mediump float attenuation = 0.0;
	mediump vec3 light = vec3(0.0);
	for(int i = 0; i < 3; i++)
	{
		posdiff = lightPosition[i].xyz-outPosition;
		attenuation = max((lightPosition[i].w-length(posdiff))/lightPosition[i].w, 0.0);
		light += lightColor[i]*dot(normal, normalize(posdiff))*attenuation;
	}
	
	mediump vec4 color0 = texture2D(mTexture0, outTexcoord);
	color0.rgb *= light;
	gl_FragColor = color0;
}
