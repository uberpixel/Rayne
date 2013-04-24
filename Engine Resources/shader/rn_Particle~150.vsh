//
//  rn_Particle.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform mat4 matView;

in vec3 vertPosition;
in vec4 vertColor0;
in vec2 vertTexcoord0;

out vec4 geoColor;
out vec2 geoSize;

void main()
{
	geoColor = vertColor0;
	geoSize  = vertTexcoord0;

	gl_Position = matView * vec4(vertPosition, 1.0);
}
