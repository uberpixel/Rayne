//
//  rn_UIImage.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
#ifdef RN_SUBPIXEL_ANTIALIAS
	vec3 color  = texture(mTexture0, vertTexcoord).rgb;
	fragColor0.rgb = color * ambient.rgb;
	fragColor0.a = (color.r + color.g + color.b) / 3.0;
#else
	float color = texture(mTexture0, vertTexcoord).r;
	fragColor0 = vec4(color * ambient.rgb, color);
#endif
}
