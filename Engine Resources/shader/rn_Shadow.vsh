//
//  rn_Shadow.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_SHADOW_VSH
#define RN_SHADOW_VSH

uniform mat4 lightDirectionalMatrix[4];
uniform mat4 lightSpotMatrix[4];

out vec4 vertDirLightProj[4];
out vec4 vertSpotLightProj[4];


void rn_ShadowDirectional0(vec4 position)
{
	vec4 pos = matModel*position;
	vertDirLightProj[0] = lightDirectionalMatrix[0]*pos;
	vertDirLightProj[1] = lightDirectionalMatrix[1]*pos;
	vertDirLightProj[2] = lightDirectionalMatrix[2]*pos;
	vertDirLightProj[3] = lightDirectionalMatrix[3]*pos;
}

void rn_ShadowSpot(vec4 position)
{
	vec4 pos = matModel*position;
	vertSpotLightProj[0] = lightSpotMatrix[0]*pos;
	vertSpotLightProj[1] = lightSpotMatrix[1]*pos;
	vertSpotLightProj[2] = lightSpotMatrix[2]*pos;
	vertSpotLightProj[3] = lightSpotMatrix[3]*pos;
}

#endif