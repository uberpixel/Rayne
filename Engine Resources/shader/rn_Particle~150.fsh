//
//  rn_Particle.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include "rn_Discard.fsh"

uniform sampler2D mTexture0;

#if defined(RN_SOFTPARTICLE)
	uniform sampler2D mTexture1;
	uniform vec2 clipPlanes;
#endif

in vec4 geoColor;
in vec2 geoTexcoord;

out vec4 fragColor0;

void main()
{
	vec4 color = texture(mTexture0, geoTexcoord);
	rn_Discard(color);
	
#if defined(RN_SOFTPARTICLE)
	vec2 depth;
	depth.x = texelFetch(mTexture1, ivec2(gl_FragCoord.xy), 0).r;
	depth.y = gl_FragCoord.z;
	depth = (clipPlanes.x * clipPlanes.y)/(depth*(clipPlanes.y-clipPlanes.x)-clipPlanes.y);
	float diff = depth.y-depth.x;
	color.a *= min(diff*10.0, 1.0);
#endif

	fragColor0 = geoColor * color;
	fragColor0.rgb *= fragColor0.a;
}
