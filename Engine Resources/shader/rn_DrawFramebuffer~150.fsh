//
//  rn_DrawFramebuffer.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_GammaCorrection.fsh"

uniform sampler2D targetmap0;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(targetmap0, vertTexcoord);

	fragColor0.rgb = rn_GammaCorrection(color0.rgb);
	fragColor0.a = color0.a;
}
