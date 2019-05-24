#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct particles_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct particles_fragment_in
{
    float4 in_var_COLOR [[user(locn0)]];
};

fragment particles_fragment_out particles_fragment(particles_fragment_in in [[stage_in]], float4 gl_FragCoord [[position]])
{
    particles_fragment_out out = {};
    float3 _16 = in.in_var_COLOR.xyz * in.in_var_COLOR.w;
    out.out_var_SV_TARGET = float4(_16.x, _16.y, _16.z, in.in_var_COLOR.w);
    return out;
}

