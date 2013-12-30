//
//  rn_DebugDraw.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

#ifdef RN_DEBUG_3D
in vec3 attPosition;
#else
in vec2 attPosition;
#endif
in vec4 attColor0;

out vec4 vertColor;

void main()
{
	vertColor = attColor0;

#ifdef RN_DEBUG_3D
	gl_Position = matProjViewModel * vec4(attPosition, 1.0);
#else
	gl_Position = matProjViewModel * vec4(attPosition, 1.0, 1.0);
#endif
}
