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
out vec4 vertDirLightProj[4];

void rn_ShadowDirectional0(vec4 position)
{
	vec4 pos = matModel*position;
	vertDirLightProj[0] = lightDirectionalMatrix[0]*pos;
	vertDirLightProj[1] = lightDirectionalMatrix[1]*pos;
	vertDirLightProj[2] = lightDirectionalMatrix[2]*pos;
	vertDirLightProj[3] = lightDirectionalMatrix[3]*pos;
}

#endif