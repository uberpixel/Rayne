//
//  rn_SurfaceNormals.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

in vec3 vertSurfaceNormal;
in float vertSurfaceDepth;

out vec4 fragColor0;

void main()
{
	fragColor0 = vec4(vertSurfaceNormal, vertSurfaceDepth);
}

