//
//  rn_Sky.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform mat4 matProj;
uniform mat4 matModelInverse;

attribute vec3 vertPosition;
attribute vec2 vertTexcoord0;

varying vec2 outTexcoord;

void main()
{
	outTexcoord = vertTexcoord0;
	
	gl_Position = matProj * matModelInverse * vec4(vertPosition, 1.0);
}