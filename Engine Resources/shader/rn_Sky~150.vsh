//
//  rn_Sky.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModelInverse;

in vec3 attPosition;
in vec2 attTexcoord0;

out vec2 vertTexcoord;

void main()
{
	vertTexcoord = attTexcoord0;
	
	gl_Position = matProj * matModelInverse * vec4(attPosition, 1.0);
}