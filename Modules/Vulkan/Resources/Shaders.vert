//
//  Shaders.glsl
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 450

layout (binding = 0) uniform UBO 
{
	mat4 modelViewProjectionMatrix;
} ubo;

layout (location = 0) in vec3 position;

void main()
{
	gl_Position = ubo.modelViewProjectionMatrix*vec4(position/10.0, 1.0);
}
