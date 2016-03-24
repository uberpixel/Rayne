//
//  Shaders.glsl
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 450

//mat4 modelViewProjectionMatrix;

layout (location = 0) in vec3 position;

void main()
{
	vec3 pos = position;
	gl_Position = vec4(pos, 1.0);
}
