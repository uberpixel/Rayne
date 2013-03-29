//
//  rn_CopyFramebuffer.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

attribute vec2 vertPosition;
attribute vec2 vertTexcoord0;

varying vec2 texcoord;

void main()
{
	texcoord = vertTexcoord0;
	gl_Position = vec4(vertPosition, 0.0, 1.0);
}
