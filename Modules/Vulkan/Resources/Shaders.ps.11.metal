#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_fragmentUniforms
{
    float4 ambientColor;
    float4 diffuseColor;
};

struct gouraud_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct gouraud_fragment_in
{
    float3 in_var_NORMAL [[user(locn1)]];
    float4 in_var_COLOR0 [[user(locn2)]];
    float2 in_var_TEXCOORD0 [[user(locn3)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _55;
    _55 = float4(0.0);
    for (uint _58 = 0u; _58 < 1u; )
    {
        _55 += float4(fast::clamp(dot(normalize(in.in_var_NORMAL), float3(1.0)), 0.0, 1.0));
        _58++;
        continue;
    }
    float4 _65 = _55;
    _65.w = 1.0;
    out.out_var_SV_TARGET = ((fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0)) * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _65);
    return out;
}

