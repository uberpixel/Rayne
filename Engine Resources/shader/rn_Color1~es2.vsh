//
//  rn_Color1.vsh
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

varying vec4 outColor;

void main()
{
	outColor = color0;

	gl_Position = matProj * matModel * vec4(position, 1.0);
}
