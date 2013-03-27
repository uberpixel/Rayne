//
//  rn_Color1.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec3 vertPosition;

void main()
{
	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}
