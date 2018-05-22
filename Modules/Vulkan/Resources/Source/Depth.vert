//
//  Depth.vert
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 450

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 modelViewProjectionMatrix;
} ubo;

layout (location = 0) in vec3 inPosition;
//layout (location = 2) in vec2 inTexcoords;

//layout (location = 0) out vec2 outTexcoords;

void main()
{
//	outTexcoords = inTexcoords;
	gl_Position = ubo.modelViewProjectionMatrix*vec4(inPosition, 1.0);
}
