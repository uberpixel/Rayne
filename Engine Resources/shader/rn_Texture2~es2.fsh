//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;

varying vec2 outTexcoord;

void main()
{
	vec4 color0 = texture2D(mTexture0, outTexcoord);
	vec4 color1 = texture2D(mTexture1, outTexcoord);

	gl_FragColor = color0 * color1;
}
