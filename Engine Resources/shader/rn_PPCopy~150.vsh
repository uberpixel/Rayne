//
//  rn_PPCopy.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform vec4 targetmap0Info;

in vec2 attPosition;
in vec2 attTexcoord0;

out vec2 vertTexcoord;

void main()
{
	#if defined(RN_DOWNSAMPLE)
		vertTexcoord = attTexcoord0+targetmap0Info.xy;
	#else
		vertTexcoord = attTexcoord0;
	#endif
	gl_Position = vec4(attPosition, 0.0, 1.0);
}
