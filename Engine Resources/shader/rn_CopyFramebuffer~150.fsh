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

void main()
{
	vec4 color0 = texture(targetmap0, vertTexcoord);
	fragColor0 = color0; // pow(color0, vec4(0.454545455));
}
