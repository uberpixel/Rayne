//
//  rn_CopyFramebuffer.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D targetmap0;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(targetmap0, texcoord);
	fragColor0 = pow(color0, vec4(0.454545455));
}
