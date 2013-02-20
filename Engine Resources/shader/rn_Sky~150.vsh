//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModelInverse;

in vec3 vertPosition;
in vec2 vertTexcoord0;

out vec2 outTexcoord;

void main()
{
	outTexcoord = vertTexcoord0;
	
	gl_Position = matProj * matModelInverse * vec4(vertPosition, 1.0);
}