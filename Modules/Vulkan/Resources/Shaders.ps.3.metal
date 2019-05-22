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
    float3 in_var_POSITION [[user(locn0)]];
    float3 in_var_NORMAL [[user(locn1)]];
    float4 in_var_COLOR0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], depth2d_array<float> directionalShadowTexture [[texture(0)]], sampler directionalShadowSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _93;
    _93 = float4(0.0);
    float4 _109;
    float _354;
    for (uint _96 = 0u; _96 < fragmentUniforms.directionalLightsCount; _93 += (_109 * _354), _96++)
    {
        _109 = fragmentUniforms.directionalLights[_96].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_96].direction.xyz), 0.0, 1.0);
        for (;;)
        {
            if ((int(_96) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
            {
                _354 = 1.0;
                break;
            }
            uint _122;
            _122 = 4294967295u;
            float4 _81[4];
            for (uint _125 = 0u; _125 < fragmentUniforms.directionalShadowMatricesCount; )
            {
                _81[_125] = fragmentUniforms.directionalShadowMatrices[_125] * float4(in.in_var_POSITION, 1.0);
                float3 _142 = _81[_125].xyz / float3(_81[_125].w);
                _81[_125] = float4(_142.x, _142.y, _142.z, _81[_125].w);
                _122 = ((((_122 > _125) && (abs(_81[_125].x) < 1.0)) && (abs(_81[_125].y) < 1.0)) && (abs(_81[_125].z) < 1.0)) ? _125 : _122;
                _125++;
                continue;
            }
            if (_122 == 4294967295u)
            {
                _354 = 1.0;
                break;
            }
            _81[_122].y *= (-1.0);
            float2 _170 = _81[_122].xy * 0.5;
            _81[_122] = float4(_170.x, _170.y, _81[_122].z, _81[_122].w);
            float2 _175 = _81[_122].xy + float2(0.5);
            _81[_122] = float4(_175.x, _175.y, _81[_122].z, _81[_122].w);
            _81[_122].w = float(_122);
            if (_122 < 3u)
            {
                float3 _326 = float3(_81[_122].xyw);
                float3 _334 = float3(_81[_122].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _342 = float3(_81[_122].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _349 = float3(_81[_122].xy + fragmentUniforms.directionalShadowInfo, _81[_122].w);
                _354 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _326.xy, uint(round(_326.z)), _81[_122].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _334.xy, uint(round(_334.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _342.xy, uint(round(_342.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _349.xy, uint(round(_349.z)), _81[_122].z, level(0.0))) * 0.25;
                break;
            }
            else
            {
                float3 _195 = float3(_81[_122].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _203 = float3(_81[_122].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _211 = float3(_81[_122].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _219 = float3(_81[_122].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _227 = float3(_81[_122].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _235 = float3(_81[_122].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _243 = float3(_81[_122].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _251 = float3(_81[_122].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _259 = float3(_81[_122].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _267 = float3(_81[_122].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _273 = float3(_81[_122].xyw);
                float3 _281 = float3(_81[_122].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _289 = float3(_81[_122].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _297 = float3(_81[_122].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _305 = float3(_81[_122].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_122].w);
                float3 _312 = float3(_81[_122].xy + fragmentUniforms.directionalShadowInfo, _81[_122].w);
                _354 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _195.xy, uint(round(_195.z)), _81[_122].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _203.xy, uint(round(_203.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _211.xy, uint(round(_211.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _219.xy, uint(round(_219.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _227.xy, uint(round(_227.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _235.xy, uint(round(_235.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _243.xy, uint(round(_243.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _251.xy, uint(round(_251.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _259.xy, uint(round(_259.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _267.xy, uint(round(_267.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _273.xy, uint(round(_273.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _281.xy, uint(round(_281.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _289.xy, uint(round(_289.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _297.xy, uint(round(_297.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _305.xy, uint(round(_305.z)), _81[_122].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _312.xy, uint(round(_312.z)), _81[_122].z, level(0.0))) * 0.0625;
                break;
            }
        }
    }
    float4 _356 = _93;
    _356.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _356);
    return out;
}

