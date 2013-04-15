//
//  rn_Color1.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"

in vec3 vertPosition;
in vec2 vertTexcoord0;

out vec2 vertTexcoord;
//out vec4 projPos;

void main()
{
	vertTexcoord = vertTexcoord0;
	vec4 pos = rn_Animate(vec4(vertPosition, 1.0));
	gl_Position = matModel * pos;
//	projPos = matProjViewModel * pos;
}
