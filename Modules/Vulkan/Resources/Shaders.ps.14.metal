#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_fragmentUniforms
{
    float4 ambientColor;
    float4 diffuseColor;
    float2 alphaToCoverageClamp;
};

struct gouraud_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct gouraud_fragment_in
{
    float4 in_var_COLOR0 [[user(locn1)]];
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _64;
    for (;;)
    {
        float4 _49 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _55 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _49.w);
        float4 _56 = _49;
        _56.w = _55;
        if (_55 < 0.001000000047497451305389404296875)
        {
            _64 = _56;
            break;
        }
        _64 = (_56 * in.in_var_COLOR0) * fragmentUniforms.ambientColor;
        break;
    }
    out.out_var_SV_TARGET = _64;
    return out;
}

