//
//  Test.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;


uniform mat4 matProj;
uniform mat4 matModel;


attribute vec3 position;
attribute vec4 color0;
attribute vec2 texcoord0;

varying vec4 color;

varying vec2 texcoord;


void main()
{
	color = color0;
	texcoord = texcoord0*10.0;
	
	gl_Position = matProj * matModel * vec4(position, 1.0);
}

