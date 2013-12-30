//
//  rn_SurfaceNormals.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"

in vec3 attPosition;
in vec3 attNormal;

out vec3 vertSurfaceNormal;
out float vertSurfaceDepth;

uniform vec2 clipPlanes;

void main()
{
	float clipfar = clipPlanes.y;
	float clipnear = clipPlanes.x;

	vec4 position = rn_Animate(vec4(attPosition, 1.0));
	vec4 normal   = rn_Animate(vec4(attNormal, 0.0));

	normal.w = 0.0;
	position.w = 1.0;

	vertSurfaceNormal = normalize((matViewModelInverse * normal).xyz);

	vertSurfaceDepth = (matViewModel * vec4(attPosition, 1.0)).z;
	vertSurfaceDepth = (-vertSurfaceDepth - clipnear) / (clipfar - clipnear);

	gl_Position = matProjViewModel * position;
}
