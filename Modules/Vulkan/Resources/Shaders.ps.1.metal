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
    float4 _45;
    _45 = float4(0.0);
    for (uint _48 = 0u; _48 < fragmentUniforms.directionalLightsCount; )
    {
        _45 += (fragmentUniforms.directionalLights[_48].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_48].direction.xyz), 0.0, 1.0));
        _48++;
        continue;
    }
    float4 _61 = _45;
    _61.w = 1.0;
    out.out_var_SV_TARGET = fragmentUniforms.diffuseColor * (fragmentUniforms.ambientColor + _61);
    return out;
}

