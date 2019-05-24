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
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _61;
    _61 = float4(0.0);
    for (uint _64 = 0u; _64 < fragmentUniforms.directionalLightsCount; )
    {
        _61 += (fragmentUniforms.directionalLights[_64].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_64].direction.xyz), 0.0, 1.0));
        _64++;
        continue;
    }
    float4 _77 = _61;
    _77.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0)) * (fragmentUniforms.ambientColor + _77);
    return out;
}

