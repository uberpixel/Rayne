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
    float spacer;
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
    float4 _41;
    _41 = float4(0.0);
    for (uint _44 = 0u; _44 < 1u; )
    {
        _41 += (fragmentUniforms.directionalLights[_44].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_44].direction.xyz), 0.0, 1.0));
        _44++;
        continue;
    }
    float4 _57 = _41;
    _57.w = 1.0;
    out.out_var_SV_TARGET = fragmentUniforms.diffuseColor * (fragmentUniforms.ambientColor + _57);
    return out;
}

