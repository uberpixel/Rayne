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
    float4 in_var_COLOR0 [[user(locn2)]];
    float2 in_var_TEXCOORD0 [[user(locn3)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _98;
    for (;;)
    {
        float4 _62 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _68 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _62.w);
        float4 _69 = _62;
        _69.w = _68;
        if (_68 < 0.001000000047497451305389404296875)
        {
            _98 = _69;
            break;
        }
        float4 _77;
        _77 = float4(0.0);
        for (uint _80 = 0u; _80 < 1u; )
        {
            _77 += (fragmentUniforms.directionalLights[_80].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_80].direction.xyz), 0.0, 1.0));
            _80++;
            continue;
        }
        float4 _93 = _77;
        _93.w = 1.0;
        _98 = (_69 * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _93);
        break;
    }
    out.out_var_SV_TARGET = _98;
    return out;
}

