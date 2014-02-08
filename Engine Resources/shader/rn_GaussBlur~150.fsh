//
//  rn_GaussBlur.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150

#ifndef RN_KERNELSIZE
#define RN_KERNELSIZE 4.5
#endif

precision highp float;

uniform sampler2D targetmap0;
uniform vec4 frameSize;
uniform float kernelWeights[10];

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	fragColor0 = vec4(0.0);
	int n = 0;
	for(float i = -RN_KERNELSIZE; i <= RN_KERNELSIZE; i += 1.0)
	{
	#ifdef RN_BLURX
		fragColor0 += texture(targetmap0, vertTexcoord+vec2(frameSize.x*i, 0.0))*kernelWeights[n];
	#else
		fragColor0 += texture(targetmap0, vertTexcoord+vec2(0.0, frameSize.y*i))*kernelWeights[n];
	#endif
		n++;
	}
}