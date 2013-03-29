//
//  rn_Texture1Discard.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform sampler2D mTexture0;

varying vec2 outTexcoord;

void main()
{
	vec4 color0 = texture2D(mTexture0, outTexcoord);
	if(color0.a < 0.3)
		discard;
	gl_FragColor = color0;
}
