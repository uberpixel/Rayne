#version 150

precision highp float;
layout(triangles) in;
layout(triangle_strip, max_vertices=512) out;

uniform mat4 matProj;
uniform mat4 matModel;
uniform float time;

in vec2 texcoordgeo[3];
out vec2 texcoord;

/*void tesselate(vec4 pos1, vec4 pos2, vec4 pos3, int i)
{
	vec4 v1 = (pos1+pos2)*0.5;
	vec4 v2 = (pos1+pos3)*0.5;
	vec4 v3 = (pos2+pos3)*0.5;

	gl_Position = matProj * matModel * pos1;
	color = vec4(1.0, 0.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * v2;
	color = vec4(0.0, 1.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * v1;
	color = vec4(0.0, 0.0, 1.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * v3;
	color = vec4(1.0, 0.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * pos2;
	color = vec4(0.0, 1.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * v3;
	color = vec4(0.0, 0.0, 1.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * pos3;
	color = vec4(1.0, 0.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = matProj * matModel * v2;
	color = vec4(0.0, 1.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	EndPrimitive();
}*/

vec4 v0, v01, v02;

void produceVertex(float s, float t)
{
	gl_Position = v0 + s*v01 + t*v02;
	texcoord = vec2(gl_Position.x/64.0+0.5, gl_Position.y/64.0+0.5);
	gl_Position.z += sin(time*4.0+gl_Position.x*0.2)*5.0;
/*	gl_Position.z *= cos(time*2.0+gl_Position.y*0.4)*5.0;
	gl_Position.x *= cos(time*2.0+gl_Position.y*0.4)*1.0;*/
	gl_Position = matProj * matModel * gl_Position;
	EmitVertex();
}

void main()
{
//	tesselate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, 0);


	int level = 4;
	int i,j;
	v0 = gl_in[0].gl_Position;
	v01 = (gl_in[1].gl_Position - gl_in[0].gl_Position);
	v02 = (gl_in[2].gl_Position - gl_in[0].gl_Position);
	int numLayers = 1 << level;
	float ds = 1.0 / float(numLayers);
	float dt = 1.0 / float(numLayers);
	float t = 0.0;
	for (i = 0; i < numLayers; i++)
	{
		float s = 0.0;
		for (j = 0; j < numLayers-i; j++)
		{
			produceVertex(s, t);
			produceVertex(s, t+dt);
			s += ds;
		}
		produceVertex(s, t);
		t += dt;
		EndPrimitive();
	}

	
/*	gl_Position = gl_in[0].gl_Position;
	gl_Position = matProj * matModel * gl_Position;
	color = vec4(1.0, 0.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = gl_in[1].gl_Position;
	gl_Position = matProj * matModel * gl_Position;
	color = vec4(0.0, 1.0, 0.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	gl_Position = gl_in[2].gl_Position;
	gl_Position = matProj * matModel * gl_Position;
	color = vec4(0.0, 0.0, 1.0, 1.0);
	texcoord = vec2(0.0);
	EmitVertex();
	EndPrimitive();*/

/*	for(int i=0; i<3; i++)
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
	EndPrimitive();*/
}