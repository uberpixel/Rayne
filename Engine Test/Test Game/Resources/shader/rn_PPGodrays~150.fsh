//
//  rn_PPGodrays.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

#include <shader/rn_Matrices.vsh>

uniform sampler2D targetmap0;
uniform sampler2D mTexture0;

#define NUM_SAMPLES 30
#define Weight 1.0
#define Decay 0.97
#define Exposure 0.02
#define Density 0.2

#if defined(RN_LIGHTING)
	#if defined(RN_DIRECTIONAL_LIGHTS)
		uniform int lightDirectionalCount;
		uniform vec3 lightDirectionalDirection[RN_DIRECTIONAL_LIGHTS];
		uniform vec4 lightDirectionalColor[RN_DIRECTIONAL_LIGHTS];
	#endif
#endif

in vec2 vertTexcoord;
out vec4 fragColor0;

void main()
{
	fragColor0 = texture(targetmap0, vertTexcoord);

#if defined(RN_LIGHTING)
	#if defined(RN_DIRECTIONAL_LIGHTS)
		vec4 lightProjection = matProjView*vec4(lightDirectionalDirection[0], 0.0);
		vec2 lightPosition = lightProjection.xy/lightProjection.w*0.5+0.5;

		// Calculate vector from pixel to light source in screen space.
		vec2 deltaTexCoord = (vertTexcoord - lightPosition);
		vec2 tempDelta = deltaTexCoord;

		//fragColor0.rgb = vec3(length(deltaTexCoord));

		// Divide by number of samples and scale by control factor.
		deltaTexCoord *= 1.0 / float(NUM_SAMPLES) * Density;

		// Store initial sample.
		vec4 color = texture(mTexture0, vertTexcoord);
		color *= color.a > 0.9999999? 1.0:0.0;

		float len = length(color.rgb);
		color /= len > 5.0? len/5.0 : 1.0;

		// Set up illumination decay factor.
		float illuminationDecay = 1.0;

		vec2 texCoord = vertTexcoord;

		// Evaluate summation from Equation 3 NUM_SAMPLES iterations.
		for (int i = 0; i < NUM_SAMPLES; i++)
		{
			// Step sample location along ray.
			texCoord -= deltaTexCoord;

			// Retrieve sample at new location.
			vec4 sample = texture(mTexture0, texCoord);
			sample *= sample.a > 0.9999999? 1.0:0.0;
			float len = length(sample.rgb);
			sample /= len > 5.0? len/5.0 : 1.0;

			//float screenfactor = max(min((0.5-length(tempDelta))*2.0, 1.0), 0.0);

			// Apply sample attenuation scale/decay factors.
			sample *= illuminationDecay * Weight;// * screenfactor;

			// Accumulate combined color.
			color += sample;

			// Update exponential decay factor.
			illuminationDecay *= Decay;
		}

		// Output final color with a further scale control factor.
		float screenfactor = max(min(2.0-length(lightPosition*2.0-1.0), 1.0), 0.0);
		fragColor0.rgb += color.rgb * Exposure * screenfactor;
	#endif
#endif
}