//
//  rn_Shadow.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_SHADOW_VSH
#define RN_SHADOW_VSH

#if defined(RN_DIRECTIONAL_SHADOWS)
uniform mat4 lightDirectionalMatrix[RN_DIRECTIONAL_SHADOWS];
out vec4 vertDirLightProj[RN_DIRECTIONAL_SHADOWS];
#endif

uniform mat4 lightSpotMatrix[4];
out vec4 vertSpotLightProj[4];

#if defined(RN_DIRECTIONAL_SHADOWS)
void rn_ShadowDirectional0(vec4 position)
{
	for(int i = 0; i < RN_DIRECTIONAL_SHADOWS; i++)
	{
		vertDirLightProj[i] = lightDirectionalMatrix[i]*position;
	}
}
#endif

void rn_ShadowSpot(vec4 position)
{
	vertSpotLightProj[0] = lightSpotMatrix[0]*position;
	vertSpotLightProj[1] = lightSpotMatrix[1]*position;
	vertSpotLightProj[2] = lightSpotMatrix[2]*position;
	vertSpotLightProj[3] = lightSpotMatrix[3]*position;
}

#endif