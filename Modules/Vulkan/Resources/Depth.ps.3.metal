#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_FragmentUniforms
{
    float2 alphaToCoverageClamp;
};

struct depth_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct depth_fragment_in
{
    float2 in_var_TEXCOORD0 [[user(locn0)]];
};

fragment depth_fragment_out depth_fragment(depth_fragment_in in [[stage_in]], constant type_FragmentUniforms& FragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    depth_fragment_out out = {};
    float4 _33 = texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
    float4 _40 = _33;
    _40.w = smoothstep(FragmentUniforms.alphaToCoverageClamp.x, FragmentUniforms.alphaToCoverageClamp.y, _33.w);
    out.out_var_SV_TARGET = _40;
    return out;
}

