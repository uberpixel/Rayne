//
//  Test.gsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150

precision highp float;
layout(triangles) in;
layout(triangle_strip, max_vertices=256) out;

uniform mat4 matProj;
uniform mat4 matModel;
uniform float time;

in vec2 texcoordgeo[3];
out vec2 texcoord;

vec4 v0, v01, v02;

void produceVertex(float s, float t)
{
	vec4 position = v0 + s*v01 + t*v02;

	gl_Position = matProj * matModel * position;
	texcoord = vec2(position.xy/64.0+0.5) * vec2(1.0, -1.0);
	
	EmitVertex();
}

void main()
{
	int level = 1;
	
	v0 = gl_in[0].gl_Position;
	v01 = (gl_in[1].gl_Position - gl_in[0].gl_Position);
	v02 = (gl_in[2].gl_Position - gl_in[0].gl_Position);
	
	int numLayers = 1 << level;
	float ds = 1.0 / float(numLayers);
	float dt = 1.0 / float(numLayers);
	float t = 0.0;
	
	for(int i = 0; i < numLayers; i++)
	{
		float s = 0.0;
		
		for(int j = 0; j < numLayers-i; j++)
		{
			produceVertex(s, t);
			produceVertex(s, t+dt);
			s += ds;
		}
		
		produceVertex(s, t);
		t += dt;
		
		EndPrimitive();
	}
}