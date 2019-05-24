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
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], sampler linearRepeatSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _99;
    for (;;)
    {
        float4 _62 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _68 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _62.w);
        float4 _69 = _62;
        _69.w = _68;
        if (_68 < 0.001000000047497451305389404296875)
        {
            _99 = _69;
            break;
        }
        float4 _78;
        _78 = float4(0.0);
        for (uint _81 = 0u; _81 < fragmentUniforms.directionalLightsCount; )
        {
            _78 += (fragmentUniforms.directionalLights[_81].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_81].direction.xyz), 0.0, 1.0));
            _81++;
            continue;
        }
        float4 _94 = _78;
        _94.w = 1.0;
        _99 = _69 * (fragmentUniforms.ambientColor + _94);
        break;
    }
    out.out_var_SV_TARGET = _99;
    return out;
}

