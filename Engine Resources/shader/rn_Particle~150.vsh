//
//  rn_Particle.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matView;

in vec3 attPosition;
in vec4 attColor0;
in vec2 attTexcoord0;

out vec4 vertColor;
out vec2 vertSize;

void main()
{
	vertColor = attColor0;
	vertSize  = attTexcoord0;

	gl_Position = matView * vec4(attPosition, 1.0);
}
