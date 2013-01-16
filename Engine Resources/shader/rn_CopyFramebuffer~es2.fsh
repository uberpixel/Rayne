//
//  rn_CopyFramebuffer.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision mediump float;

uniform sampler2D targetmap;
varying lowp vec2 texcoord;

void main()
{
	gl_FragColor = texture2D(targetmap, texcoord);
}
