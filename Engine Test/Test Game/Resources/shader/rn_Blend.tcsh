#version 400

#include <shader/rn_Matrices.vsh>

// number of CPs in patch
layout (vertices = 3) out;

in vec3 vertPosition[];
in vec4 vertTexcoord[];

in vec3 vertNormal[];
in vec3 vertBitangent[];
in vec3 vertTangent[];

uniform vec3 viewPosition;
uniform vec4 frameSize;

out vec3 tcPosition[];
out vec4 tcTexcoord[];

out vec3 tcNormal[];
out vec3 tcBitangent[];
out vec3 tcTangent[];

void main()
{
	tcTexcoord[gl_InvocationID] = vertTexcoord[gl_InvocationID];
	tcPosition[gl_InvocationID] = vertPosition[gl_InvocationID];

	tcNormal[gl_InvocationID] = vertNormal[gl_InvocationID];
	tcBitangent[gl_InvocationID] = vertBitangent[gl_InvocationID];
	tcTangent[gl_InvocationID] = vertTangent[gl_InvocationID];

	float dist0 = (distance(viewPosition, (vertPosition[1]+vertPosition[2])*0.5)-5.0)/30.0;
	float fac0 = pow(2, floor(max((1.0-dist0)*6.99, 0.0)));

	float dist1 = (distance(viewPosition, (vertPosition[0]+vertPosition[2])*0.5)-5.0)/30.0;
	float fac1 = pow(2, floor(max((1.0-dist1)*6.99, 0.0)));

	float dist2 = (distance(viewPosition, (vertPosition[1]+vertPosition[0])*0.5)-5.0)/30.0;
	float fac2 = pow(2, floor(max((1.0-dist2)*6.99, 0.0)));

	gl_TessLevelOuter[0] = fac0;
	gl_TessLevelOuter[1] = fac1;
	gl_TessLevelOuter[2] = fac2;
	gl_TessLevelInner[0] = min(min(fac0, fac1), fac2);

/*	if(any(greaterThan(abs(gl_in[0].gl_Position.xy), vec2(1.0))) && any(greaterThan(abs(gl_in[1].gl_Position.xy), vec2(1.0))) && any(greaterThan(abs(gl_in[2].gl_Position.xy), vec2(1.0))))
	{
		gl_TessLevelOuter[0] = 0.0;
		gl_TessLevelOuter[1] = 0.0;
		gl_TessLevelOuter[2] = 0.0;
		gl_TessLevelInner[0] = 0.0;
	}*/
}