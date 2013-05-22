//
//  rn_PPCopy.fsh
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
	vec4 color = texture(targetmap0, vertTexcoord);
	fragColor0 = color;
}