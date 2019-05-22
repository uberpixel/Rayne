#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_fragmentUniforms
{
    float4 diffuseColor;
};

struct sky_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct sky_fragment_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment sky_fragment_out sky_fragment(sky_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    sky_fragment_out out = {};
    out.out_var_SV_TARGET = texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0) * fragmentUniforms.diffuseColor;
    return out;
}

