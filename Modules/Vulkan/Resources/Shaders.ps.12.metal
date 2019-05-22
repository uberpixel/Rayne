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
    float2 in_var_TEXCOORD0 [[user(locn1)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _68;
    for (;;)
    {
        float4 _54 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _60 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _54.w);
        float4 _61 = _54;
        _61.w = _60;
        if (_60 < 0.001000000047497451305389404296875)
        {
            _68 = _61;
            break;
        }
        _68 = _61 * fragmentUniforms.ambientColor;
        break;
    }
    out.out_var_SV_TARGET = _68;
    return out;
}

