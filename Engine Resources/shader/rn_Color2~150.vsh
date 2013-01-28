//
//  rn_Color2.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matProj;
uniform mat4 matModel;

in vec3 position;
in vec4 color0;
in vec4 color1;

out vec4 outColor0;
out vec4 outColor1;

void main()
{
	outColor0 = color0;
	outColor1 = color1;

	gl_Position = matProj * matModel * vec4(position, 1.0);
}
