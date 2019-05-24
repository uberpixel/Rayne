#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct pp_blit_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct pp_blit_fragment_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment pp_blit_fragment_out pp_blit_fragment(pp_blit_fragment_in in [[stage_in]], texture2d<float> texture0 [[texture(0)]], sampler linearClampSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    pp_blit_fragment_out out = {};
    out.out_var_SV_TARGET = texture0.sample(linearClampSampler, in.in_var_TEXCOORD0);
    return out;
}

