//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform mat4 matProjViewModel;
uniform mat4 matModel;

attribute vec3 vertPosition;
attribute vec3 vertNormal;
attribute vec2 vertTexcoord0;

varying vec2 outTexcoord;
varying vec3 outNormal;
varying vec3 outPosition;

void main()
{
	outTexcoord = vertTexcoord0;
	outNormal = (matModel * vec4(vertNormal, 0.0)).xyz;
	outPosition = (matModel * vec4(vertPosition, 1.0)).xyz;
	
	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}