//
//  rn_Water.vsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Matrices.vsh"

uniform float time;

in vec3 attPosition;
out vec3 vertProjPos;
out vec3 vertPosition;
out vec4 vertTexcoord;
out vec4 vertTexcoord2;

void main()
{
	vertPosition = (matModel * vec4(attPosition, 1.0)).xyz;
	vertTexcoord.xy = vertPosition.xz*0.2+time*0.05;
	vertTexcoord.zw = -vertPosition.xz*0.2+time*0.05;
	vertTexcoord2.xy = vec2(-1.0, 1.0)*vertPosition.xz*1.0-time*0.2;
	vertTexcoord2.zw = vec2(1.0, -1.0)*vertPosition.xz*1.0-time*0.2;
	gl_Position = matProjViewModel * vec4(attPosition, 1.0);
	vertProjPos = gl_Position.xyw;
}
