//
//  rn_PPCombine.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D targetmap0;
uniform sampler2D mTexture0;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(targetmap0, vertTexcoord);
	vec4 color1 = texture(mTexture0, vertTexcoord);

#if defined(MODE_GRAYSCALE)
	vec4 result = color0 * vec4(color1.r);
#elif defined(MODE_RGBA_MULTIPLY)
	vec4 result = color0 * color1;
#else
	vec4 result = color0 + color1;
#endif

	fragColor0 = result;
}