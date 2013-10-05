//
//  rn_Shadow.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_SHADOW_FSH
#define RN_SHADOW_FSH

#ifdef RN_LIGHTING
uniform sampler2DArrayShadow lightDirectionalDepth;
uniform samplerCube lightPointDepth0;
uniform samplerCube lightPointDepth1;
uniform samplerCube lightPointDepth2;
uniform samplerCube lightPointDepth3;
uniform samplerCube lightPointDepth4;
uniform samplerCube lightPointDepth5;

uniform vec4 frameSize;
in vec4 vertDirLightProj[4];

uniform float lightPointRanges[10];

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

//returns the shadow factor for the first directional light
float rn_ShadowDirectional0()
{
	vec3 proj[4];
	proj[0] = vertDirLightProj[0].xyz/vertDirLightProj[0].w;
	proj[1] = vertDirLightProj[1].xyz/vertDirLightProj[1].w;
	proj[2] = vertDirLightProj[2].xyz/vertDirLightProj[2].w;
	proj[3] = vertDirLightProj[3].xyz/vertDirLightProj[3].w;
	
	bvec3 inMap[3];
	inMap[2] = lessThan(proj[2]*proj[2], vec3(1.0));
	inMap[1] = lessThan(proj[1]*proj[1], vec3(1.0));
	inMap[0] = lessThan(proj[0]*proj[0], vec3(1.0));
	
	float mapToUse = 3;
	
	if(inMap[2].x && inMap[2].y && inMap[2].z)
		mapToUse = 2;
	
	if(inMap[1].x && inMap[1].y && inMap[1].z)
		mapToUse = 1;
	
	if(inMap[0].x && inMap[0].y && inMap[0].z)
		mapToUse = 0;
	
	vec4 projected = vec4(proj[int(mapToUse)]*0.5+0.5, mapToUse);
//	return rn_textureOffset(lightDirectionalDepth, projected, vec2(0.0));
	return rn_ShadowPCF2x2(lightDirectionalDepth, projected);
//	return rn_ShadowPCF4x4(lightDirectionalDepth, projected);
}

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
	else if(index == 4)
		return texture(lightPointDepth4, pos).r;
	else if(index == 5)
		return texture(lightPointDepth5, pos).r;
	else return 1.0;
}
/*
float rn_ShadowPointPCF2x2(int index, vec4 pos)
{
	float result = rn_ShadowPointTextureCubeArrayShadow(index, pos);
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec4(0.01, 0.01, 0.01, 0.0));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec4(0.01, 0.01, 0.01, 0.0));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec4(0.01, -0.01, 0.01, 0.0));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec4(0.01, -0.01, -0.01, 0.0));
	return result*0.2;
}*/

float rn_ShadowPointPCF2x2(int index, vec3 pos)
{
	float result = rn_ShadowPointTextureCubeArrayShadow(index, pos);
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec3(0.01, 0.01, 0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec3(0.01, 0.01, 0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec3(0.01, -0.01, 0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec3(0.01, -0.01, 0.01));
	
/*	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec3(0.01, 0.01, -0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec3(0.01, 0.01, -0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos+vec3(0.01, -0.01, -0.01));
	result += rn_ShadowPointTextureCubeArrayShadow(index, pos-vec3(0.01, -0.01, -0.01));
	*/
	return result*0.2*lightPointRanges[index];
}

float rn_ShadowPoint(int light, vec3 dir)
{
	float occluder = /*rn_ShadowPointPCF2x2(light, dir);*/ rn_ShadowPointTextureCubeArrayShadow(light, dir)*lightPointRanges[light];
	float receiver = length(dir);
	return min(1.0, max(0.0, exp((occluder-receiver)*15.0)));
	
//	return rn_ShadowPointTextureCubeArrayShadow(light, lookup);
//	return rn_ShadowPointPCF2x2(light, lookup);
}

float rn_ShadowSpot0()
{
	return 1.0;
}

#else
#define rn_ShadowDirectional0() (1.0)
#define rn_ShadowPoint0(dir) (1.0)
#define rn_ShadowSpot0() (1.0)
#endif

#endif
