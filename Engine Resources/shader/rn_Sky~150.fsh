//
//  rn_Sky.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;

uniform vec4 ambient;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, vertTexcoord)*ambient;
	fragColor0 = color0;
}
