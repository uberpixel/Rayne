//
//  rn_Texture1DiscardLight.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vhs"

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexcoord0;

out vec2 outTexcoord;
out vec3 outNormal;
out vec3 outPosition;

void main()
{
	vec4 pos = rn_Animate(vertPosition);
	vec4 normal = rn_Animate(vertNormal);

	normal.w = 0.0;

	outNormal = (matModel * normal).xyz;
	outPosition = (matModel * pos).xyz;
	outTexcoord = vertTexcoord0;
	
	gl_Position = matProjViewModel * pos;
}