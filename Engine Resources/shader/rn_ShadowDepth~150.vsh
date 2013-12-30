//
//  rn_Color1.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"

#if defined(RN_VEGETATION)
	uniform float time;
#endif

in vec3 attPosition;
in vec2 attTexcoord0;

out vec2 vertTexcoord;

void main()
{
	vertTexcoord = attTexcoord0;
	vec4 position = rn_Animate(vec4(attPosition, 1.0));
	
	#if defined(RN_VEGETATION)
		position.x += sin(time)*position.y*0.02;
		position.z += cos(time)*position.y*0.02;
	#endif
	
	gl_Position = matModel * position;
}
