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
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _95;
    for (;;)
    {
        float4 _60 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _66 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _60.w);
        float4 _67 = _60;
        _67.w = _66;
        if (_66 < 0.001000000047497451305389404296875)
        {
            _95 = _67;
            break;
        }
        float4 _74;
        _74 = float4(0.0);
        for (uint _77 = 0u; _77 < 1u; )
        {
            _74 += (fragmentUniforms.directionalLights[_77].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_77].direction.xyz), 0.0, 1.0));
            _77++;
            continue;
        }
        float4 _90 = _74;
        _90.w = 1.0;
        _95 = _67 * (fragmentUniforms.ambientColor + _90);
        break;
    }
    out.out_var_SV_TARGET = _95;
    return out;
}

