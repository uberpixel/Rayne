//
//  rn_Billboard.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Lighting.vsh"

in vec2 vertPosition;
in vec2 vertTexcoord0;
in float vertNormal;

out vec2 outTexcoord;

void main()
{
	outTexcoord = vertTexcoord0;

	rn_Lighting(vec3(vertPosition, 0.0), vec3(1.0));
	gl_Position = matProjViewModel * vec4(vertPosition, 0.0, 1.0);
}
