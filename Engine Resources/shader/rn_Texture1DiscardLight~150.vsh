//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProjViewModel;
uniform mat4 matModel;

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexcoord0;

out vec2 outTexcoord;
out vec3 outNormal;
out vec3 outPosition;

void main()
{
	outTexcoord = vertTexcoord0;
	outNormal = (matModel * vec4(vertNormal, 0.0)).xyz;
	outPosition = (matModel * vec4(vertPosition, 1.0)).xyz;
	
	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}