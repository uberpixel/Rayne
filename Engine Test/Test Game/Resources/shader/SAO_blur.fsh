#version 150 // -*- c++ -*-
#line 4
/** 
  \file SAO_blur.pix
  \author Morgan McGuire and Michael Mara, NVIDIA Research

  \brief 7-tap 1D cross-bilateral blur using a packed depth key

  DX11 HLSL port by Leonardo Zide, Treyarch
  
  Open targetmap0 under the "BSD" license: http://www.opentargetmap0.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in targetmap0 and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of targetmap0 code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//////////////////////////////////////////////////////////////////////////////////////////////
// Tunable Parameters:

/** Increase to make depth edges crisper. Decrease to reduce flicker. */
#define EDGE_SHARPNESS     (1.0)

/** Step in 2-pixel intervals since we already blurred against neighbors in the
    first AO pass.  This constant can be increased while R decreases to improve
    performance at the expense of some dithering artifacts. 
    
    Morgan found that a scale of 3 left a 1-pixel checkerboard grid that was
    unobjectionable after shading was applied but eliminated most temporal incoherence
    from using small numbers of sample taps.
    */
#define SCALE               (2)

/** Filter radius in pixels. This will be multiplied by SCALE. */
#define R                   (4)


//////////////////////////////////////////////////////////////////////////////////////////////

/** Type of data to read from targetmap0.  This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_TYPE        float

/** Swizzle to use to extract the channels of targetmap0. This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_COMPONENTS   r

#define VALUE_IS_KEY       0

/** Channel encoding the bilateral key value (which must not be the same as VALUE_COMPONENTS) */
#define KEY_COMPONENTS     gb


#if __VERSION__ >= 330
// Gaussian coefficients
const float gaussian[R + 1] = 
//    float[](0.356642, 0.239400, 0.072410, 0.009869);
//    float[](0.398943, 0.241971, 0.053991, 0.004432, 0.000134);  // stddev = 1.0
    float[](0.153170, 0.144893, 0.122649, 0.092902, 0.062970);  // stddev = 2.0
//      float[](0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108); // stddev = 3.0
#endif

uniform sampler2D   targetmap0;

/** (1, 0) or (0, 1)*/
#if defined(RN_BLURX)
	#define axis ivec2(1, 0)
#else
	#define axis ivec2(0, 1)
#endif

#if __VERSION__ == 120
#   define          texelFetch texelFetch2D
#else
out vec4            fragColor0;
#endif

#define  result         fragColor0.VALUE_COMPONENTS
#define  keyPassThrough fragColor0.KEY_COMPONENTS

/** Returns a number on (0, 1) */
float unpackKey(vec2 p) {
    return p.x * (256.0 / 257.0) + p.y * (1.0 / 257.0);
}


void main() {
#   if __VERSION__ < 330
        float gaussian[R + 1];
#       if R == 3
            gaussian[0] = 0.153170; gaussian[1] = 0.144893; gaussian[2] = 0.122649; gaussian[3] = 0.092902;  // stddev = 2.0
#       elif R == 4
            gaussian[0] = 0.153170; gaussian[1] = 0.144893; gaussian[2] = 0.122649; gaussian[3] = 0.092902; gaussian[4] = 0.062970;  // stddev = 2.0
#       elif R == 6
            gaussian[0] = 0.111220; gaussian[1] = 0.107798; gaussian[2] = 0.098151; gaussian[3] = 0.083953; gaussian[4] = 0.067458; gaussian[5] = 0.050920; gaussian[6] = 0.036108;
#       endif
#   endif

    ivec2 ssC = ivec2(gl_FragCoord.xy);

    vec4 temp = texelFetch(targetmap0, ssC, 0);
    
    keyPassThrough = temp.KEY_COMPONENTS;
    float key = unpackKey(keyPassThrough);

    VALUE_TYPE sum = temp.VALUE_COMPONENTS;

    if (key == 1.0) { 
        // Sky pixel (if you aren't using depth keying, disable this test)
        result = sum;
        return;
    }

    // Base weight for depth falloff.  Increase this for more blurriness,
    // decrease it for better edge discrimination
    float BASE = gaussian[0];
    float totalWeight = BASE;
    sum *= totalWeight;

   
    for (int r = -R; r <= R; ++r) {
        // We already handled the zero case above.  This loop should be unrolled and the static branch optimized out,
        // so the IF statement has no runtime cost
        if (r != 0) {
            temp = texelFetch(targetmap0, ssC + axis * (r * SCALE), 0);
            float      tapKey = unpackKey(temp.KEY_COMPONENTS);
            VALUE_TYPE value  = temp.VALUE_COMPONENTS;
            
            // spatial domain: offset gaussian tap
            float weight = 0.3 + gaussian[abs(r)];
            
            // range domain (the "bilateral" weight). As depth difference increases, decrease weight.
            weight *= max(0.0, 1.0
                - (EDGE_SHARPNESS * 2000.0) * abs(tapKey - key)
                );

            sum += value * weight;
            totalWeight += weight;
        }
    }
 
    const float epsilon = 0.0001;
    result = sum / (totalWeight + epsilon);	
}
