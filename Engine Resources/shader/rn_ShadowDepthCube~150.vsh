//
//  rn_Color1.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"
#include "rn_Animation.vsh"

#if defined(RN_VEGETATION)
	uniform float time;
#endif

uniform vec2 clipPlanes;
uniform vec3 viewPosition;

in vec3 attPosition;
in vec2 attTexcoord0;

out vec2 vertTexcoord;
out float vertCamDist;

void main()
{
	vertTexcoord = attTexcoord0;
	vec4 position = rn_Animate(vec4(attPosition, 1.0));
	
	#if defined(RN_VEGETATION)
		position.x += sin(time)*position.y*0.02;
		position.z += cos(time)*position.y*0.02;
	#endif
	
	vec4 worldpos = matModel * position;
	vertCamDist = length(worldpos.xyz-viewPosition)/clipPlanes.y;
	gl_Position = matProjViewModel * position;
}
