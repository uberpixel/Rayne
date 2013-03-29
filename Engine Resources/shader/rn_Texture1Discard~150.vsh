//
//  rn_Texture1Discard.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"
#include "rn_Lightning.vsh"

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexcoord0;

out vec2 outTexcoord;

void main()
{
	outTexcoord = vertTexcoord0;

	vec3 position = rn_Lightning(matModel, vertPosition, vertNormal);
	gl_Position = matProjViewModel * vec4(position, 1.0);
}
