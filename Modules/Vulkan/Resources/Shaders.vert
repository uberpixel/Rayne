//
//  Shaders.glsl
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 140

//mat4 modelViewProjectionMatrix;

in vec3 position;

void main()
{
	gl_Position = vec4(position, 1.0);
}
