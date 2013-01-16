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
layout(triangle_strip, max_vertices=27) out;

uniform mat4 matProj;
uniform mat4 matModel;

in vec2 texcoordgeo[3];
in vec4 colorgeo[3];
out vec2 texcoord;
out vec4 color;

void main()
{
	for(int i=0; i<3; i++)
	{
		gl_Position = gl_in[i].gl_Position;
		gl_Position.z -= 32;
		gl_Position = matProj * matModel * gl_Position;
		texcoord = texcoordgeo[i];
		color = colorgeo[i];
		EmitVertex();
	}
	EndPrimitive();

	for(int i=0; i<3; i++)
	{
		gl_Position.x = gl_in[i].gl_Position.x;
		gl_Position.y = gl_in[i].gl_Position.y;
		gl_Position.z = gl_in[i].gl_Position.z+32.0;
		gl_Position.w = 1.0;
		gl_Position = matProj * matModel * gl_Position;
		texcoord = texcoordgeo[i];
		color = colorgeo[i];
		EmitVertex();
	}
	EndPrimitive();

	for(int i=0; i<3; i++)
	{
		gl_Position.x = gl_in[i].gl_Position.x;
		gl_Position.y = gl_in[i].gl_Position.z+32;
		gl_Position.z = gl_in[i].gl_Position.y;
		gl_Position.w = 1.0;
		gl_Position = matProj * matModel * gl_Position;
		texcoord = texcoordgeo[i];
		color = colorgeo[i];
		EmitVertex();
	}
	EndPrimitive();

	for(int i=0; i<3; i++)
	{
		gl_Position.x = gl_in[i].gl_Position.x;
		gl_Position.y = gl_in[i].gl_Position.z-32;
		gl_Position.z = gl_in[i].gl_Position.y;
		gl_Position.w = 1.0;
		gl_Position = matProj * matModel * gl_Position;
		texcoord = texcoordgeo[i];
		color = colorgeo[i];
		EmitVertex();
	}
	EndPrimitive();
}