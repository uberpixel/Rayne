//
//  rn_DebugDraw.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec3 attPosition;
in vec4 attColor0;

out vec4 vertColor;

void main()
{
	vertColor = attColor0;
	gl_Position = matProjViewModel * vec4(attPosition, 1.0);
}
