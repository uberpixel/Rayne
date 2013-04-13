//
//  rn_Lighting.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#define RN_LIGHTING_VSH

#ifdef RN_LIGHTING

uniform mat4 lightDirectionalMatrix[10];

out vec3 outLightNormal;
out vec3 outLightPosition;
out vec4 outDirLightProj[4];

vec3 rn_Lighting(mat4 model, vec3 position, vec3 normal)
{
#ifdef RN_ANIMATION_VSH
	vec4 pos = rn_Animate(vec4(position, 1.0));
	vec4 norm = rn_Animate(vec4(normal, 0.0));
	norm.w = 0.0;
	 
	outLightNormal = (model * norm).xyz;
	outLightPosition = (model * pos).xyz;
	
	mat4 biasMatrix = mat4(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);
	outDirLightProj[0] = /*biasMatrix**/lightDirectionalMatrix[0]*matModel*pos;
	outDirLightProj[1] = /*biasMatrix**/lightDirectionalMatrix[1]*matModel*pos;
	outDirLightProj[2] = /*biasMatrix**/lightDirectionalMatrix[2]*matModel*pos;
	outDirLightProj[3] = /*biasMatrix**/lightDirectionalMatrix[3]*matModel*pos;

	return pos.xyz;
#else
	outLightNormal = (model * vec4(normal, 0.0)).xyz;
	outLightPosition = (model * vec4(position, 1.0)).xyz;
	
	mat4 biasMatrix = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
	);
	outDirLightProj[0] = biasMatrix*lightDirectionalMatrix[0]*matModel*vec4(position, 1.0);
	outDirLightProj[1] = biasMatrix*lightDirectionalMatrix[1]*matModel*vec4(position, 1.0);
	outDirLightProj[2] = biasMatrix*lightDirectionalMatrix[2]*matModel*vec4(position, 1.0);
	outDirLightProj[3] = biasMatrix*lightDirectionalMatrix[3]*matModel*vec4(position, 1.0);

	return position;
#endif
}

#else

#ifdef RN_ANIMATION_VSH

vec3 rn_Lighting(mat4 model, vec3 position, vec3 normal)
{
	vec4 pos = rn_Animate(vec4(position, 1.0));
	return pos.xyz;
}

#else
#define rn_Lighting(model, position, normal) (position)
#endif

#endif
