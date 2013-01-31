//
//  rn_Color1.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProjViewModel;

in vec3 position;
in vec4 color0;

out vec4 outColor;

void main()
{
	outColor = color0;

	gl_Position = matProjViewModel * vec4(position, 1.0);
}
