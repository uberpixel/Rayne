//
//  rn_LightTileSample.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec2 vertPosition;
in vec2 vertTexcoord0;

out vec2 texcoord;

void main()
{
	texcoord = vertTexcoord0;
	gl_Position = vec4(vertPosition, 0.0, 1.0);
}