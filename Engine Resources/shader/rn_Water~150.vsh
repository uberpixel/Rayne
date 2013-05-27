//
//  rn_Water.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec3 attPosition;
out vec2 vertTexcoord;

void main()
{
	vertTexcoord = (matModel * vec4(attPosition, 1.0)).xz;
	gl_Position = matProjViewModel * vec4(attPosition, 1.0);
}
