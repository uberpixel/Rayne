//
//  rn_Particle.gsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 matProjViewModel;
uniform mat4 matViewInverse;

in vec2 vertSize[1];
in vec4 vertColor[1];

out vec4 geoColor;
out vec2 geoTexcoord;

void main()
{
	vec4 base = gl_in[0].gl_Position;
	vec2 size = vertSize[0];

	gl_Position = matProjViewModel * (base + matViewInverse * vec4(size.x, size.y, 0.0, 0.0));
	geoTexcoord = vec2(0.0, 1.0);
	geoColor = vertColor[0];
	EmitVertex();

	gl_Position = matProjViewModel * (base + matViewInverse * vec4(-size.x, size.y, 0.0, 0.0));
	geoTexcoord = vec2(0.0, 0.0);
	geoColor = vertColor[0];
	EmitVertex();

	gl_Position = matProjViewModel * (base + matViewInverse * vec4(size.x, -size.y, 0.0, 0.0));
	geoTexcoord = vec2(1.0, 1.0);
	geoColor = vertColor[0];
	EmitVertex();

	gl_Position = matProjViewModel * (base + matViewInverse * vec4(-size.x, -size.y, 0.0, 0.0));
	geoTexcoord = vec2(1.0, 0.0);
	geoColor = vertColor[0];
	EmitVertex();

	EndPrimitive();
}
