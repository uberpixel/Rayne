//
//  Sky.vert
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 450

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 modelViewMatrix;
	mat4 projectionMatrix;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 2) in vec2 inTexcoords;

layout (location = 0) out vec2 outTexcoords;

void main()
{
	outTexcoords = inTexcoords;
	vec4 rotatedPosition = ubo.modelViewMatrix * vec4(inPosition, 0.0);
	rotatedPosition.w = 1.0;

	gl_Position = (ubo.projectionMatrix*rotatedPosition).xyww;
}
