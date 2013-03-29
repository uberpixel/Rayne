//
//  rn_CopyFramebuffer.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision mediump float;

uniform sampler2D targetmap0;
varying lowp vec2 texcoord;

void main()
{
	gl_FragColor = texture2D(targetmap0, texcoord);
}
