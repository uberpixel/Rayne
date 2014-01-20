//
//  rn_Sky.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModelInverse;

in vec3 attPosition;
in vec2 attTexcoord0;

#if defined(RN_ATMOSPHERE)
out vec3 vertDirToCam;
#else
out vec2 vertTexcoord;
#endif

void main()
{
#if defined(RN_ATMOSPHERE)
	vertDirToCam = attPosition;
#else
	vertTexcoord = attTexcoord0;
#endif
	
	gl_Position = matProj * matModelInverse * vec4(attPosition, 1.0);
}