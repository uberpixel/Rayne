//
//  rn_Color2.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

precision highp float;

uniform mat4 matProjViewModel;

attribute vec3 position;
attribute vec4 color0;
attribute vec4 color1;

varying vec4 outColor0;
varying vec4 outColor1;

void main()
{
	outColor0 = color0;
	outColor1 = color1;

	gl_Position = matProjViewModel * vec4(position, 1.0);
}
