//
//  TestPP2.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

uniform mat4 matProj;
uniform mat4 matModel;

attribute vec2 position;
attribute vec2 texcoord0;

varying vec2 texcoord;

void main()
{
	texcoord = texcoord0;
	gl_Position = vec4(position, 0.0, 1.0);
}

