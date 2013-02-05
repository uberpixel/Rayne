//
//  Test.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform mat4 matProjViewModel;

attribute vec3 vertPosition;
attribute vec4 vertColor0;
attribute vec2 vertTexcoord0;

varying vec4 color;

varying vec2 texcoord;


void main()
{
	color = vertColor0;
	texcoord = vertTexcoord0*10.0;
	
	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}

