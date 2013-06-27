//
//  rn_DrawFramebuffer.fsh
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

void main()
{
#ifdef RN_GAMMA_CORRECTION
	vec4 color0 = texture(targetmap0, vertTexcoord);

	fragColor0.rgb = pow(color0.rgb, vec3(0.454545455));
	fragColor0.a = color0.a;
#else
	fragColor0 = texture(targetmap0, vertTexcoord);
#endif
}
