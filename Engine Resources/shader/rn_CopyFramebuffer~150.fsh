//
//  rn_CopyFramebuffer.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D targetmap0;

in vec2 vertTexcoord;
out vec4 fragColor0;

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 1.0; //11.2

vec3 Uncharted2Tonemap(vec3 x)
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	vec4 color0 = texture(targetmap0, vertTexcoord);
	
	vec3 tonemapped = Uncharted2Tonemap(1.0*color0.rgb)/Uncharted2Tonemap(vec3(W));
	fragColor0.rgb = pow(tonemapped, vec3(1.0/2.2));
	fragColor0.a = color0.a;
}