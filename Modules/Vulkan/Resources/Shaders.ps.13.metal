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
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _84;
    for (;;)
    {
        float4 _56 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _62 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _56.w);
        float4 _63 = _56;
        _63.w = _62;
        if (_62 < 0.001000000047497451305389404296875)
        {
            _84 = _63;
            break;
        }
        float4 _69;
        _69 = float4(0.0);
        for (uint _72 = 0u; _72 < 1u; )
        {
            _69 += float4(fast::clamp(dot(normalize(in.in_var_NORMAL), float3(1.0)), 0.0, 1.0));
            _72++;
            continue;
        }
        float4 _79 = _69;
        _79.w = 1.0;
        _84 = _63 * (fragmentUniforms.ambientColor + _79);
        break;
    }
    out.out_var_SV_TARGET = _84;
    return out;
}

