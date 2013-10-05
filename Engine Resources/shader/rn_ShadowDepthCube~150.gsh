//
//  rn_Color1.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150 core
precision highp float;

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

in vec2 vertTexcoord[3];
in float vertCamDist[3];
out vec2 geoTexcoord;
out float geoCamDist;

void main(void)
{
	vec4 pos[3];
	pos[0] = gl_in[0].gl_Position;
	pos[1] = gl_in[1].gl_Position;
	pos[2] = gl_in[2].gl_Position;
	
	//x+
	gl_Layer = 0;
	geoTexcoord = vertTexcoord[0];
	geoCamDist = vertCamDist[0];
	gl_Position = vec4(-pos[0].z, pos[0].y, pos[0].x, -pos[0].x);
	EmitVertex();
	
	gl_Layer = 0;
	geoTexcoord = vertTexcoord[2];
	geoCamDist = vertCamDist[2];
	gl_Position = vec4(-pos[2].z, pos[2].y, pos[2].x, -pos[2].x);
	EmitVertex();
	
	gl_Layer = 0;
	geoTexcoord = vertTexcoord[1];
	geoCamDist = vertCamDist[1];
	gl_Position = vec4(-pos[1].z, pos[1].y, pos[1].x, -pos[1].x);
	EmitVertex();
	EndPrimitive();
	
	//x-
	gl_Layer = 1;
	geoTexcoord = vertTexcoord[0];
	geoCamDist = vertCamDist[0];
	gl_Position = vec4(pos[0].z, pos[0].y, -pos[0].x, pos[0].x);
	EmitVertex();
	
	gl_Layer = 1;
	geoTexcoord = vertTexcoord[2];
	geoCamDist = vertCamDist[2];
	gl_Position = vec4(pos[2].z, pos[2].y, -pos[2].x, pos[2].x);
	EmitVertex();
	
	gl_Layer = 1;
	geoTexcoord = vertTexcoord[1];
	geoCamDist = vertCamDist[1];
	gl_Position = vec4(pos[1].z, pos[1].y, -pos[1].x, pos[1].x);
	EmitVertex();
	EndPrimitive();
	
	//y+
	gl_Layer = 2;
	geoTexcoord = vertTexcoord[0];
	geoCamDist = vertCamDist[0];
	gl_Position = vec4(-pos[0].x, pos[0].z, pos[0].y, -pos[0].y);
	EmitVertex();
	
	gl_Layer = 2;
	geoTexcoord = vertTexcoord[2];
	geoCamDist = vertCamDist[2];
	gl_Position = vec4(-pos[2].x, pos[2].z, pos[2].y, -pos[2].y);
	EmitVertex();
	
	gl_Layer = 2;
	geoTexcoord = vertTexcoord[1];
	geoCamDist = vertCamDist[1];
	gl_Position = vec4(-pos[1].x, pos[1].z, pos[1].y, -pos[1].y);
	EmitVertex();
	EndPrimitive();
	
	//y-
	gl_Layer = 3;
	geoTexcoord = vertTexcoord[0];
	geoCamDist = vertCamDist[0];
	gl_Position = vec4(-pos[0].x, -pos[0].z, -pos[0].y, pos[0].y);
	EmitVertex();
	
	gl_Layer = 3;
	geoTexcoord = vertTexcoord[2];
	geoCamDist = vertCamDist[2];
	gl_Position = vec4(-pos[2].x, -pos[2].z, -pos[2].y, pos[2].y);
	EmitVertex();
	
	gl_Layer = 3;
	geoTexcoord = vertTexcoord[1];
	geoCamDist = vertCamDist[1];
	gl_Position = vec4(-pos[1].x, -pos[1].z, -pos[1].y, pos[1].y);
	EmitVertex();
	EndPrimitive();
	
	//z+
	gl_Layer = 4;
	geoTexcoord = vertTexcoord[0];
	geoCamDist = vertCamDist[0];
	gl_Position = vec4(-pos[0].x, pos[0].y, -pos[0].z, pos[0].z);
	EmitVertex();
	
	gl_Layer = 4;
	geoTexcoord = vertTexcoord[2];
	geoCamDist = vertCamDist[2];
	gl_Position = vec4(-pos[2].x, pos[2].y, -pos[2].z, pos[2].z);
	EmitVertex();
	
	gl_Layer = 4;
	geoTexcoord = vertTexcoord[1];
	geoCamDist = vertCamDist[1];
	gl_Position = vec4(-pos[1].x, pos[1].y, -pos[1].z, pos[1].z);
	EmitVertex();
	EndPrimitive();
	
	//z-
	gl_Layer = 5;
	geoTexcoord = vertTexcoord[0];
	geoCamDist = vertCamDist[0];
	gl_Position = vec4(pos[0].x, pos[0].y, pos[0].z, -pos[0].z);
	EmitVertex();
	
	gl_Layer = 5;
	geoTexcoord = vertTexcoord[2];
	geoCamDist = vertCamDist[2];
	gl_Position = vec4(pos[2].x, pos[2].y, pos[2].z, -pos[2].z);
	EmitVertex();
	
	gl_Layer = 5;
	geoTexcoord = vertTexcoord[1];
	geoCamDist = vertCamDist[1];
	gl_Position = vec4(pos[1].x, pos[1].y, pos[1].z, -pos[1].z);
	EmitVertex();
	EndPrimitive();
}