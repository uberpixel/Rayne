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
    float4 in_var_COLOR0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _48;
    _48 = float4(0.0);
    for (uint _51 = 0u; _51 < fragmentUniforms.directionalLightsCount; )
    {
        _48 += (fragmentUniforms.directionalLights[_51].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_51].direction.xyz), 0.0, 1.0));
        _51++;
        continue;
    }
    float4 _64 = _48;
    _64.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _64);
    return out;
}

