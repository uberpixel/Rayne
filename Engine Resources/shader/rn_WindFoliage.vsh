//
//  rn_WindFoliage.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Lighting.vsh"

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexcoord0;

uniform float time;

out vec2 outTexcoord;

void main()
{
	outTexcoord = vertTexcoord0;

	vec3 position = rn_Lighting(matModel, vertPosition, vertNormal);

	

	//if(position.y > 0.0)
	//{
		vec3 parameters = vec3(10.0, time, 1.0);

		float force = parameters.x / 25.0 * (25.0 / 300.0);
		float direction = time; //parameters.y / 100.0 * 2.0 * 3.14;
		float oscillation = sin(time * parameters.z);

		if(oscillation < 0.0)
			oscillation *= 0.33;

		position.x += oscillation * (force * position.y) * 0.8;
		position.z += oscillation * (force * position.y) * 0.5;
		position.y -= abs(oscillation) * pow(force, 2.0) * position.y;
	//}

	gl_Position = matProjViewModel * vec4(position, 1.0);
}
