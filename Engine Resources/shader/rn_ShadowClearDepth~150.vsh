//
//  rn_ShadowClearDepth.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
in vec3 attPosition;

void main()
{
	gl_Position = vec4(attPosition.xy, 1.0, 1.0);
}
