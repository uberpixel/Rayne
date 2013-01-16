//
//  rn_CopyFramebuffer.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D targetmap;

in vec2 texcoord;
out vec4 fragColor0;

void main()
{
	fragColor0 = texture(targetmap, texcoord);
}
