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
	mat4 modelMatrix;
	vec4 ambient;
	vec4 diffuse;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormals;
layout (location = 2) in vec2 inTexcoords;

layout (location = 0) out vec2 outTexcoords;
layout (location = 1) out vec3 outNormals;
layout (location = 2) out vec4 outAmbient;
layout (location = 3) out vec4 outDiffuse;

void main()
{
	outTexcoords = inTexcoords;
	outNormals = (ubo.modelMatrix * vec4(inNormals, 0.0)).xyz;
	outAmbient = ubo.ambient;
	outDiffuse = ubo.diffuse;

	gl_Position = ubo.modelViewProjectionMatrix*vec4(inPosition, 1.0);
}
