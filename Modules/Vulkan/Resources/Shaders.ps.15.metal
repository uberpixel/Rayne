#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_fragmentUniforms
{
    float4 ambientColor;
    float4 diffuseColor;
    float2 alphaToCoverageClamp;
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
    float4 _87;
    for (;;)
    {
        float4 _58 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _64 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _58.w);
        float4 _65 = _58;
        _65.w = _64;
        if (_64 < 0.001000000047497451305389404296875)
        {
            _87 = _65;
            break;
        }
        float4 _72;
        _72 = float4(0.0);
        for (uint _75 = 0u; _75 < 1u; )
        {
            _72 += float4(fast::clamp(dot(normalize(in.in_var_NORMAL), float3(1.0)), 0.0, 1.0));
            _75++;
            continue;
        }
        float4 _82 = _72;
        _82.w = 1.0;
        _87 = (_65 * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _82);
        break;
    }
    out.out_var_SV_TARGET = _87;
    return out;
}

