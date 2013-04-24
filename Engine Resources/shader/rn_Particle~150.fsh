//
//  rn_Particle.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Discard.fsh"

uniform sampler2D mTexture0;

in vec4 fragColor;
in vec2 texCoord;

out vec4 fragColor0;

void main()
{
	vec4 color = texture(mTexture0, texCoord);
	rn_Discard(color);

	fragColor0 = fragColor * color;
}
