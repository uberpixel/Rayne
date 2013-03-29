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

in vec3 vertPosition;
in vec4 vertColor0;

out vec4 outColor;

void main()
{
	outColor = vertColor0;

	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}
