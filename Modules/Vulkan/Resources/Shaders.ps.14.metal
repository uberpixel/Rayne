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
    uint directionalShadowMatricesCount;
    uint directionalLightsCount;
    float2 directionalShadowInfo;
    float4x4 directionalShadowMatrices[4];
    LightDirectional directionalLights[5];
};

struct gouraud_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

struct gouraud_fragment_in
{
    float4 in_var_COLOR0 [[user(locn1)]];
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _71;
    for (;;)
    {
        float4 _56 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _62 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _56.w);
        float4 _63 = _56;
        _63.w = _62;
        if (_62 < 0.001000000047497451305389404296875)
        {
            _71 = _63;
            break;
        }
        _71 = (_63 * in.in_var_COLOR0) * fragmentUniforms.ambientColor;
        break;
    }
    out.out_var_SV_TARGET = _71;
    return out;
}

