#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct depth_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

fragment depth_fragment_out depth_fragment()
{
    depth_fragment_out out = {};
    out.out_var_SV_TARGET = float4(1.0);
    return out;
}

