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
#endif

in vec4 geoColor;
in vec2 geoTexcoord;

out vec4 fragColor0;

void main()
{
	vec4 color = texture(mTexture0, geoTexcoord);
	rn_Discard(color);
	
#if defined(RN_SOFTPARTICLE)
	float depth = texelFetch(mTexture1, ivec2(gl_FragCoord.xy), 0).r;
	float diff = depth-gl_FragCoord.z;
	color.a *= min(diff*200.0, 1.0);
#endif

	fragColor0 = geoColor * color;
	fragColor0.rgb *= fragColor0.a;
}
