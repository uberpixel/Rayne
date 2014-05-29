//
//  rn_UIView.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform vec4 diffuse;

out vec4 fragColor0;

void main()
{
	fragColor0 = diffuse;
}
