//
//  rn_LightTileSampleFirst.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;
layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform sampler2D depthmap;
uniform vec4 depthmapinfo;

uniform vec4 frameSize;
uniform vec2 clipPlanes;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	vec4 depth1 = texelFetch(depthmap, ivec2(min(gl_FragCoord.x*2, depthmapinfo.z-1.0), min(gl_FragCoord.y*2, depthmapinfo.w-1.0)), 0);
	vec4 depth2 = texelFetch(depthmap, ivec2(min(gl_FragCoord.x*2+1, depthmapinfo.z-1.0), min(gl_FragCoord.y*2+1, depthmapinfo.w-1.0)), 0);
	vec4 depth3 = texelFetch(depthmap, ivec2(min(gl_FragCoord.x*2, depthmapinfo.z-1.0), min(gl_FragCoord.y*2+1, depthmapinfo.w-1.0)), 0);
	vec4 depth4 = texelFetch(depthmap, ivec2(min(gl_FragCoord.x*2+1, depthmapinfo.z-1.0), min(gl_FragCoord.y*2, depthmapinfo.w-1.0)), 0);
	
	fragColor0.r = min(depth1.r, min(depth2.r, min(depth3.r, depth4.r)));
	fragColor0.g = max(depth1.r, max(depth2.r, max(depth3.r, depth4.r)));
	
	fragColor0.rg = (clipPlanes.x * clipPlanes.y)/(fragColor0.rg*(clipPlanes.y-clipPlanes.x)-clipPlanes.y);
}
