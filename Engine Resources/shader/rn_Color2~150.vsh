//
//  rn_Color2.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec3 vertPosition;
in vec4 vertColor0;
in vec4 vertColor1;

out vec4 outColor0;
out vec4 outColor1;

void main()
{
	outColor0 = vertColor0;
	outColor1 = vertColor1;

	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}
