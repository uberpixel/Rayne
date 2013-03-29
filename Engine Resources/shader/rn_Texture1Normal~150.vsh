//
//  rn_Texture1.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

in vec3 vertPosition;
in vec3 vertNormal;
in vec4 vertTangent;
in vec2 vertTexcoord0;

out vec2 outTexcoord;
out mat3 outTangentToWorldMatrix;

out vec3 outLightNormal;
out vec3 outLightPosition;

void main()
{
	outTexcoord = vertTexcoord0;
	
	outLightNormal = (matModel * vec4(vertNormal, 0.0)).xyz;
	outLightPosition = (matModel * vec4(vertPosition, 1.0)).xyz;
	
	outTangentToWorldMatrix[0] = (matModel*vec4(vertTangent.xyz, 0.0)).xyz;
	outTangentToWorldMatrix[2] = (matModel*vec4(vertNormal, 0.0)).xyz;
	outTangentToWorldMatrix[1] = cross(outTangentToWorldMatrix[0], outTangentToWorldMatrix[2])*vertTangent.w;
	
	//outTangentToWorldMatrix = inverse(outTangentToWorldMatrix);

	gl_Position = matProjViewModel * vec4(vertPosition, 1.0);
}
