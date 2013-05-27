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
	fragColor0 = (dot(color0.rgb, vec3(0.299, 0.587, 0.114))>0.5)?color0:vec4(vec3(0.0), 1.0);
}