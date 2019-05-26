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
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _39;
    _39 = float4(0.0);
    for (uint _42 = 0u; _42 < 1u; )
    {
        _39 += float4(fast::clamp(dot(normalize(in.in_var_NORMAL), float3(1.0)), 0.0, 1.0));
        _42++;
        continue;
    }
    float4 _49 = _39;
    _49.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _49);
    return out;
}

