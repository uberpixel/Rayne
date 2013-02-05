//
//  Test.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProjViewModel;

in vec3 vertPosition;
in vec4 vertColor0;
in vec2 vertTexcoord0;

out vec4 color;

out vec2 texcoord;


void main()
{
	color = vertColor0;
	texcoord = vertTexcoord0*10.0;
	
	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}

