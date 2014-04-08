//
//  rn_Shadow.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_SHADOW_FSH
#define RN_SHADOW_FSH

#if defined(RN_LIGHTING)
uniform sampler2DArrayShadow lightDirectionalDepth;
uniform samplerCube lightPointDepth0;
uniform samplerCube lightPointDepth1;
uniform samplerCube lightPointDepth2;
uniform samplerCube lightPointDepth3;
uniform sampler2DShadow lightSpotDepth0;
uniform sampler2DShadow lightSpotDepth1;
uniform sampler2DShadow lightSpotDepth2;
uniform sampler2DShadow lightSpotDepth3;

uniform vec4 frameSize;

#if defined(RN_DIRECTIONAL_SHADOWS)
	in vec4 vertDirLightProj[RN_DIRECTIONAL_SHADOWS];
#endif

in vec4 vertSpotLightProj[4];

//a textureOffset lookup for a 2DArrayShader sampler
float rn_textureOffset(sampler2DArrayShadow map, vec4 loc, vec2 offset)
{
	return texture(map, vec4(offset*frameSize.xy+loc.xy, loc.wz));
}

//basic 4x4 blur, with hardware bilinear filtering if enabled
float rn_ShadowPCF2x2(sampler2DArrayShadow map, vec4 projected)
{
	float shadow = rn_textureOffset(map, projected, vec2(0.0, 0.0));
	shadow += rn_textureOffset(map, projected, vec2(1.0, 0.0));
	shadow += rn_textureOffset(map, projected, vec2(0.0, 1.0));
	shadow += rn_textureOffset(map, projected, vec2(1.0, 1.0));
	shadow *= 0.25;
	return shadow;
}

//basic 4x4 blur, with hardware bilinear filtering if enabled
float rn_ShadowPCF4x4(sampler2DArrayShadow map, vec4 projected)
{
	float shadow = rn_textureOffset(map, projected, vec2(-2.0, -2.0));
	shadow += rn_textureOffset(map, projected, vec2(-1.0, -2.0));
	shadow += rn_textureOffset(map, projected, vec2(0.0, -2.0));
	shadow += rn_textureOffset(map, projected, vec2(1.0, -2.0));
	
	shadow += rn_textureOffset(map, projected, vec2(-2.0, -1.0));
	shadow += rn_textureOffset(map, projected, vec2(-1.0, -1.0));
	shadow += rn_textureOffset(map, projected, vec2(0.0, -1.0));
	shadow += rn_textureOffset(map, projected, vec2(1.0, -1.0));
	
	shadow += rn_textureOffset(map, projected, vec2(-2.0, 0.0));
	shadow += rn_textureOffset(map, projected, vec2(-1.0, 0.0));
	shadow += rn_textureOffset(map, projected, vec2(0.0, 0.0));
	shadow += rn_textureOffset(map, projected, vec2(1.0, 0.0));
	
	shadow += rn_textureOffset(map, projected, vec2(-2.0, 1.0));
	shadow += rn_textureOffset(map, projected, vec2(-1.0, 1.0));
	shadow += rn_textureOffset(map, projected, vec2(0.0, 1.0));
	shadow += rn_textureOffset(map, projected, vec2(1.0, 1.0));
	
	shadow *= 0.0625;
	return shadow;
}

#if defined(RN_DIRECTIONAL_SHADOWS)
//returns the shadow factor for the first directional light
float rn_ShadowDirectional0()
{
	vec3 proj[RN_DIRECTIONAL_SHADOWS];
	for(int i = 0; i < RN_DIRECTIONAL_SHADOWS; i++)
	{
		proj[i] = vertDirLightProj[i].xyz/vertDirLightProj[i].w;
	}
	
	bvec3 inMap[RN_DIRECTIONAL_SHADOWS-1];
	for(int i = RN_DIRECTIONAL_SHADOWS-2; i >= 0; i--)
	{
		inMap[i] = lessThan(proj[i]*proj[i], vec3(1.0));
	}
	
	int mapToUse = RN_DIRECTIONAL_SHADOWS-1;
	
	for(int i = RN_DIRECTIONAL_SHADOWS-2; i >= 0; i--)
	{
		if(inMap[i].x && inMap[i].y && inMap[i].z)
			mapToUse = i;
	}
	
	vec4 projected = vec4(proj[mapToUse]*0.5+0.5, float(mapToUse));
//	return rn_textureOffset(lightDirectionalDepth, projected, vec2(0.0));
	return rn_ShadowPCF2x2(lightDirectionalDepth, projected);
//	return rn_ShadowPCF4x4(lightDirectionalDepth, projected);
}
#endif

float rn_ShadowPointTextureCubeArrayShadow(int index, vec3 pos)
{
	if(index == 0)
		return texture(lightPointDepth0, pos).r;
	else if(index == 1)
		return texture(lightPointDepth1, pos).r;
	else if(index == 2)
		return texture(lightPointDepth2, pos).r;
	else if(index == 3)
		return texture(lightPointDepth3, pos).r;
	else return 1.0;
}

float rn_ShadowPointPCF2x2(int index, vec3 pos)
{
	float result = rn_ShadowPointTextureCubeArrayShadow(index, pos);
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec3(0.01, 0.01, 0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec3(0.01, 0.01, 0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec3(0.01, -0.01, 0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec3(0.01, -0.01, 0.01));
	
	return result*0.2;
}

float rn_ShadowPoint(int light, vec3 dir, float range)
{
	float occluder = rn_ShadowPointTextureCubeArrayShadow(light, dir)*range;
	float receiver = length(dir);
	return min(1.0, max(0.0, exp((occluder-receiver)*15.0)));
	
//	return rn_ShadowPointTextureCubeArrayShadow(light, lookup);
//	return rn_ShadowPointPCF2x2(light, lookup);
}

float rn_ShadowSpotTextureArrayShadow(int index, vec3 pos)
{
	if(index == 0)
		return texture(lightSpotDepth0, pos);
	else if(index == 1)
		return texture(lightSpotDepth1, pos);
	else if(index == 2)
		return texture(lightSpotDepth2, pos);
	else if(index == 3)
		return texture(lightSpotDepth3, pos);
	else return 1.0;
}

float rn_ShadowSpot(int light)
{
	vec3 proj = vertSpotLightProj[light].xyz/vertSpotLightProj[light].w*0.5+0.5;
	return rn_ShadowSpotTextureArrayShadow(light, proj);
}

#else
#define rn_ShadowDirectional0() (1.0)
#define rn_ShadowPoint0(dir) (1.0)
#define rn_ShadowSpot0() (1.0)
#endif

#endif
