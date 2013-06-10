//
//  rn_UIImage.fsh
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
	float color = texture(mTexture0, vertTexcoord).r;
	vec4 result = vec4(vec3(1.0) * pow(color, 1), color);
	fragColor0 = result;
}
