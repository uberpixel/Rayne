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
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _46;
    _46 = float4(0.0);
    for (uint _49 = 0u; _49 < fragmentUniforms.directionalLightsCount; )
    {
        _46 += (fragmentUniforms.directionalLights[_49].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_49].direction.xyz), 0.0, 1.0));
        _49++;
        continue;
    }
    float4 _62 = _46;
    _62.w = 1.0;
    out.out_var_SV_TARGET = fragmentUniforms.diffuseColor * (fragmentUniforms.ambientColor + _62);
    return out;
}

