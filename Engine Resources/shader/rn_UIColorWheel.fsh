//
//  rn_UIColorWheel.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#define kPI 3.141592653589793

uniform vec4 diffuse;

in vec2 vertTexcoord;
out vec4 fragColor0;

vec3 hsv2rgb(float h, float s, float v)
{
	float hi = h * 3.0 / kPI;
	float f  = hi - floor(hi);

	vec4 components = vec4(0.0, s, s * f, s * (1.0 - f));
	components = (1.0 - components) * v;

	if(hi < -2.0)
	{
	    return components.xwy;
	}
	else if(hi < -1.0)
	{
	    return components.zxy;
	}
	else if(hi < 0.0)
	{
	    return components.yxw;
	}
	else if(hi < 1.0)
	{
	    return components.yzx;
	}
	else if(hi < 2.0)
	{
	    return components.wyx;
	}
	else
	{
	    return components.xyz;
	}
}

void main()
{
	vec2 coords = vertTexcoord * 2.0 - 1.0;

	float theta = atan(coords.y, -coords.x);
	float r = length(coords);

	float E = 0.5 * fwidth(r);

	vec4 rgba = vec4(pow(hsv2rgb(theta, r, diffuse.a), vec3(2.2)), smoothstep(1.0 + E, 1.0 - E, r));

	fragColor0 = rgba;
}
