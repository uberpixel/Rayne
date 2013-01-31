//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform mat4 matProjViewModel;

attribute vec3 position;
attribute vec2 texcoord0;

varying vec2 outTexcoord;

void main()
{
	outTexcoord = texcoord0;
	
	gl_Position = matProjViewModel * vec4(position, 1.0);
}