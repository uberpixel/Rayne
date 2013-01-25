//
//  Test.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform sampler2D mTexture0;

varying vec2 texcoord;

void main()
{
	vec4 outcolor0 = texture2D(mTexture0, texcoord);
	gl_FragColor = outcolor0;
}
