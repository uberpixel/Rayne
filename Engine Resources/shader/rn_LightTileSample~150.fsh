//
//  rn_LightTileSample.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;
layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform sampler2D targetmap0;
uniform vec4 targetmap0Info;
uniform vec4 frameSize;

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	vec4 depth1 = texelFetch(targetmap0, ivec2(min(gl_FragCoord.x*2.0, targetmap0Info.z-1.0), min(gl_FragCoord.y*2.0, targetmap0Info.w-1.0)), 0);
	vec4 depth2 = texelFetch(targetmap0, ivec2(min(gl_FragCoord.x*2.0+1.0, targetmap0Info.z-1.0), min(gl_FragCoord.y*2.0+1.0, targetmap0Info.w-1.0)), 0);
	vec4 depth3 = texelFetch(targetmap0, ivec2(min(gl_FragCoord.x*2.0, targetmap0Info.z-1.0), min(gl_FragCoord.y*2.0+1.0, targetmap0Info.w-1.0)), 0);
	vec4 depth4 = texelFetch(targetmap0, ivec2(min(gl_FragCoord.x*2.0+1.0, targetmap0Info.z-1.0), min(gl_FragCoord.y*2.0, targetmap0Info.w-1.0)), 0);
	
	fragColor0.r = max(depth1.r, max(depth2.r, max(depth3.r, depth4.r)));
	fragColor0.g = min(depth1.g, min(depth2.g, min(depth3.g, depth4.g)));
}
