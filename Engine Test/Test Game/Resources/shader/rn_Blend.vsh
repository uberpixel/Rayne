//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 400
precision highp float;

#include <shader/rn_Matrices.vsh>

in vec3 attPosition;
in vec3 attNormal;
in vec2 attTexcoord0;
in vec4 attTangent;

out vec3 vertPosition;
out vec4 vertTexcoord;

out vec3 vertNormal;
out vec3 vertBitangent;
out vec3 vertTangent;

void main()
{
	vertTexcoord = vec4(attTexcoord0*200.0, attTexcoord0);
	vertPosition = (matModel * vec4(attPosition, 1.0)).xyz;
	
	vertNormal = (matModel * vec4(attNormal, 0.0)).xyz;
	vertTangent = (matModel * vec4(attTangent.xyz, 0.0)).xyz;
	vertBitangent = cross(vertNormal, vertTangent) * attTangent.w;
	
/*	gl_Position = matProjViewModel * vec4(attPosition, 1.0);
	gl_Position.xy /= gl_Position.w;*/
}
