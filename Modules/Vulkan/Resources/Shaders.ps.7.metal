#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct LightDirectional
{
    float4 direction;
    float4 color;
};

struct type_fragmentUniforms
{
    float4 ambientColor;
    float4 diffuseColor;
    float2 alphaToCoverageClamp;
    uint directionalLightsCount;
    LightDirectional directionalLights[5];
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
    float4 _49;
    _49 = float4(0.0);
    for (uint _52 = 0u; _52 < fragmentUniforms.directionalLightsCount; )
    {
        _49 += (fragmentUniforms.directionalLights[_52].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_52].direction.xyz), 0.0, 1.0));
        _52++;
        continue;
    }
    float4 _65 = _49;
    _65.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _65);
    return out;
}

