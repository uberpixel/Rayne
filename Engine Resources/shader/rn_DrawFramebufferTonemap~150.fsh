//
//  rn_DrawFramebufferTonemape.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_GammaCorrection.fsh"

uniform sampler2D targetmap0;
uniform vec4 hdrSettings;

in vec2 vertTexcoord;
out vec4 fragColor0;

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;

vec3 Uncharted2Tonemap(vec3 x)
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	vec4 color0 = texture(targetmap0, vertTexcoord);
	
	vec3 tonemapped = Uncharted2Tonemap(hdrSettings.x*color0.rgb)/Uncharted2Tonemap(vec3(hdrSettings.y));
	fragColor0.rgb = rn_GammaCorrection(tonemapped);
	fragColor0.a = color0.a;
}
