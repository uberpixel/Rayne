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
    float2 in_var_TEXCOORD0 [[user(locn3)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _102;
    for (;;)
    {
        float4 _64 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _70 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _64.w);
        float4 _71 = _64;
        _71.w = _70;
        if (_70 < 0.001000000047497451305389404296875)
        {
            _102 = _71;
            break;
        }
        float4 _81;
        _81 = float4(0.0);
        for (uint _84 = 0u; _84 < fragmentUniforms.directionalLightsCount; )
        {
            _81 += (fragmentUniforms.directionalLights[_84].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_84].direction.xyz), 0.0, 1.0));
            _84++;
            continue;
        }
        float4 _97 = _81;
        _97.w = 1.0;
        _102 = (_71 * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _97);
        break;
    }
    out.out_var_SV_TARGET = _102;
    return out;
}

