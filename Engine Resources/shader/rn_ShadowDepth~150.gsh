//
//  rn_Color1.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150 core
precision highp float;

uniform mat4 lightDirectionalMatrix[10];
uniform mat4 matProjView;

layout (triangles) in;
layout (triangle_strip, max_vertices = 12) out;

in vec2 vertTexcoord[3];
out vec2 geoTexcoord;

void main(void)
{
	// Use backface culling to improve performance
//	vec2 d0 = projPos[1].xy - projPos[0].xy;
//	vec2 d1 = projPos[2].xy - projPos[0].xy;
	
//	if(dot(d0,d1) < 0.0)
//	{
		//pass-thru!
/*		for(int i=0; i<4; i++)
		{
			vec4 pos[3];
			pos[0] = lightDirectionalMatrix[i]*gl_in[0].gl_Position;
			pos[1] = lightDirectionalMatrix[i]*gl_in[1].gl_Position;
			pos[2] = lightDirectionalMatrix[i]*gl_in[2].gl_Position;
			
			vec3 proj[3];
			proj[0] = pos[0].xyz/pos[0].w;
			proj[1] = pos[1].xyz/pos[1].w;
			proj[2] = pos[2].xyz/pos[2].w;
			
			vec3 dist = vec3(dot(proj[0], proj[0]), dot(proj[1], proj[1]), dot(proj[2], proj[2]));
			
			vec3 zGreater = vec3(greaterThan(dist, vec3(1.0)));
			float count = dot(zGreater, vec3(1.0));
			
			if(count < 3.5)
			{
				gl_Layer = i;
				outTexcoord = vertTexcoord[0];
				gl_Position = pos[0];
				EmitVertex();
				
				gl_Layer = i;
				outTexcoord = vertTexcoord[1];
				gl_Position = pos[1];
				EmitVertex();
				
				gl_Layer = i;
				outTexcoord = vertTexcoord[2];
				gl_Position = pos[2];
				EmitVertex();
				EndPrimitive();
				
				if(count < 0.5)
					return;
			}
		}*/
//	}
	
	{
		vec4 pos[3];
		pos[0] = lightDirectionalMatrix[0]*gl_in[0].gl_Position;
		pos[1] = lightDirectionalMatrix[0]*gl_in[1].gl_Position;
		pos[2] = lightDirectionalMatrix[0]*gl_in[2].gl_Position;
		
		vec3 proj[3];
		proj[0] = pos[0].xyz/pos[0].w;
		proj[1] = pos[1].xyz/pos[1].w;
		proj[2] = pos[2].xyz/pos[2].w;
		
		vec3 dist = vec3(dot(proj[0], proj[0]), dot(proj[1], proj[1]), dot(proj[2], proj[2]));
		
		vec3 zGreater = vec3(greaterThan(dist, vec3(1.0)));
		float count = dot(zGreater, vec3(1.0));
		
		if(count < 3.5)
		{
			gl_Layer = 0;
			geoTexcoord = vertTexcoord[0];
			gl_Position = pos[0];
			EmitVertex();
			
			gl_Layer = 0;
			geoTexcoord = vertTexcoord[1];
			gl_Position = pos[1];
			EmitVertex();
			
			gl_Layer = 0;
			geoTexcoord = vertTexcoord[2];
			gl_Position = pos[2];
			EmitVertex();
			EndPrimitive();
			
			if(count < 0.5)
			return;
		}
	}
	
	{
		vec4 pos[3];
		pos[0] = lightDirectionalMatrix[1]*gl_in[0].gl_Position;
		pos[1] = lightDirectionalMatrix[1]*gl_in[1].gl_Position;
		pos[2] = lightDirectionalMatrix[1]*gl_in[2].gl_Position;
		
		vec3 proj[3];
		proj[0] = pos[0].xyz/pos[0].w;
		proj[1] = pos[1].xyz/pos[1].w;
		proj[2] = pos[2].xyz/pos[2].w;
		
		vec3 dist = vec3(dot(proj[0], proj[0]), dot(proj[1], proj[1]), dot(proj[2], proj[2]));
		
		vec3 zGreater = vec3(greaterThan(dist, vec3(1.0)));
		float count = dot(zGreater, vec3(1.0));
		
		if(count < 3.5)
		{
			gl_Layer = 1;
			geoTexcoord = vertTexcoord[0];
			gl_Position = pos[0];
			EmitVertex();
			
			gl_Layer = 1;
			geoTexcoord = vertTexcoord[1];
			gl_Position = pos[1];
			EmitVertex();
			
			gl_Layer = 1;
			geoTexcoord = vertTexcoord[2];
			gl_Position = pos[2];
			EmitVertex();
			EndPrimitive();
			
			if(count < 0.5)
			return;
		}
	}

	{
		vec4 pos[3];
		pos[0] = lightDirectionalMatrix[2]*gl_in[0].gl_Position;
		pos[1] = lightDirectionalMatrix[2]*gl_in[1].gl_Position;
		pos[2] = lightDirectionalMatrix[2]*gl_in[2].gl_Position;
		
		vec3 proj[3];
		proj[0] = pos[0].xyz/pos[0].w;
		proj[1] = pos[1].xyz/pos[1].w;
		proj[2] = pos[2].xyz/pos[2].w;
		
		vec3 dist = vec3(dot(proj[0], proj[0]), dot(proj[1], proj[1]), dot(proj[2], proj[2]));
		
		vec3 zGreater = vec3(greaterThan(dist, vec3(1.0)));
		float count = dot(zGreater, vec3(1.0));
		
		if(count < 3.5)
		{
			gl_Layer = 2;
			geoTexcoord = vertTexcoord[0];
			gl_Position = pos[0];
			EmitVertex();
			
			gl_Layer = 2;
			geoTexcoord = vertTexcoord[1];
			gl_Position = pos[1];
			EmitVertex();
			
			gl_Layer = 2;
			geoTexcoord = vertTexcoord[2];
			gl_Position = pos[2];
			EmitVertex();
			EndPrimitive();
			
			if(count < 0.5)
			return;
		}
	}
	
	{
		vec4 pos[3];
		pos[0] = lightDirectionalMatrix[3]*gl_in[0].gl_Position;
		pos[1] = lightDirectionalMatrix[3]*gl_in[1].gl_Position;
		pos[2] = lightDirectionalMatrix[3]*gl_in[2].gl_Position;
		
		vec3 proj[3];
		proj[0] = pos[0].xyz/pos[0].w;
		proj[1] = pos[1].xyz/pos[1].w;
		proj[2] = pos[2].xyz/pos[2].w;
		
		vec3 dist = vec3(dot(proj[0], proj[0]), dot(proj[1], proj[1]), dot(proj[2], proj[2]));
		
		vec3 zGreater = vec3(greaterThan(dist, vec3(1.0)));
		float count = dot(zGreater, vec3(1.0));
		
		if(count < 3.5)
		{
			gl_Layer = 3;
			geoTexcoord = vertTexcoord[0];
			gl_Position = pos[0];
			EmitVertex();
			
			gl_Layer = 3;
			geoTexcoord = vertTexcoord[1];
			gl_Position = pos[1];
			EmitVertex();
			
			gl_Layer = 3;
			geoTexcoord = vertTexcoord[2];
			gl_Position = pos[2];
			EmitVertex();
			EndPrimitive();
			
			if(count < 0.5)
			return;
		}
	}
}