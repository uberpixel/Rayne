//
//  rn_UIGradientView.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#define kPI 3.141592653589793

uniform vec4 diffuse;
uniform vec4 ambient;

uniform float angle;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	vec2  coords   = vertTexcoord - sin(angle);
	float gradient = coords.x * sin(angle) + coords.y * cos(angle);

	vec4 difference = (ambient - diffuse);

	fragColor0 = pow(diffuse + (difference * gradient), vec4(2.2));
}
