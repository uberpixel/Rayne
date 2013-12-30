//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform sampler2D mTexture0;

varying vec2 outTexcoord;

void main()
{
	vec4 color0 = texture2D(mTexture0, outTexcoord);
	gl_FragColor = color0;
}
