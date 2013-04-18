//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"
#include "rn_Lighting.vsh"

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexcoord0;

out vec2 outTexcoord;

void main()
{
	outTexcoord = vertTexcoord0;

	vec4 position = rn_Animate(vec4(vertPosition, 1.0));
	vec4 normal   = rn_Animate(vec4(vertNormal, 0.0));

	normal.w = 0.0;

	rn_Lighting(position.xyz, normal.xyz);
	gl_Position = matProjViewModel * vec4(position.xyz, 1.0);
}
