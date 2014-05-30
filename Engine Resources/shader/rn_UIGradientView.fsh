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

	fragColor0.rgb = mix(diffuse.rgb, ambient.rgb, gradient);
	fragColor0.rgb = pow(fragColor0.rgb, vec3(2.2));
	fragColor0.a = diffuse.a;
}
