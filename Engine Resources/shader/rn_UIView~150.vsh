//
//  rn_UIView.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec2 attPosition;

void main()
{
	gl_Position = matProjViewModel * vec4(attPosition, 1.0, 1.0);
}
