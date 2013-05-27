//
//  rn_CopyFramebuffer.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150

#ifndef RN_KERNELSIZE
#define RN_KERNELSIZE 4.5
#endif

precision highp float;

uniform sampler2D targetmap0;
uniform vec4 frameSize;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	fragColor0 = vec4(0.0);
	vec4 color0 = texture(targetmap0, vertTexcoord);
	for(float i = -RN_KERNELSIZE; i <= RN_KERNELSIZE; i += 1.0)
	#ifdef RN_BLURX
		fragColor0 += texture(targetmap0, vertTexcoord+vec2(frameSize.x*i, 0.0));
	#else
		fragColor0 += texture(targetmap0, vertTexcoord+vec2(0.0, frameSize.y*i));
	#endif
	
	fragColor0 *= 1.0/(RN_KERNELSIZE*2.0+1.0);
}