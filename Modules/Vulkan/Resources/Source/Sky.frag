//
//  Sky.frag
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 450

layout (set = 0, binding = 1) uniform UBO 
{
	vec4 diffuseColor;
} ubo;

layout (set = 0, binding = 2) uniform sampler linearSampler;
layout (set = 0, binding = 3) uniform texture2D colorMap;

layout (location = 0) in vec2 inTexcoords;

layout (location = 0) out vec4 fragColor0;


void main()
{
	vec4 color = texture(sampler2D(colorMap, linearSampler), inTexcoords);
	fragColor0 = color * ubo.diffuseColor;
}
