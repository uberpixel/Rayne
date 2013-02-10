//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform sampler2D mTexture0;
uniform vec4 lightPosition[2];
uniform vec3 lightColor[2];
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

		posdiff = lightPosition[0].xyz-outPosition;
		attenuation = max((lightPosition[0].w-length(posdiff))/lightPosition[0].w, 0.0);
		light += lightColor[0]*dot(normal, normalize(posdiff))*attenuation;
	
		posdiff = lightPosition[1].xyz-outPosition;
		attenuation = max((lightPosition[1].w-length(posdiff))/lightPosition[1].w, 0.0);
		light += lightColor[1]*dot(normal, normalize(posdiff))*attenuation;
	
	mediump vec4 color0 = texture2D(mTexture0, outTexcoord);
	color0.rgb *= light+0.1;
	gl_FragColor = color0;
}
