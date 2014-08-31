//
//  rn_ShadowClearDepth.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;
out vec4 fragColor0;

void main()
{
	fragColor0 = vec4(vec3(gl_FragCoord.z), 1.0);
}
