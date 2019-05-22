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
    float3 in_var_POSITION [[user(locn0)]];
    float3 in_var_NORMAL [[user(locn1)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], depth2d_array<float> directionalShadowTexture [[texture(0)]], sampler directionalShadowSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _91;
    _91 = float4(0.0);
    float4 _107;
    float _352;
    for (uint _94 = 0u; _94 < fragmentUniforms.directionalLightsCount; _91 += (_107 * _352), _94++)
    {
        _107 = fragmentUniforms.directionalLights[_94].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_94].direction.xyz), 0.0, 1.0);
        for (;;)
        {
            if ((int(_94) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
            {
                _352 = 1.0;
                break;
            }
            uint _120;
            _120 = 4294967295u;
            float4 _81[4];
            for (uint _123 = 0u; _123 < fragmentUniforms.directionalShadowMatricesCount; )
            {
                _81[_123] = fragmentUniforms.directionalShadowMatrices[_123] * float4(in.in_var_POSITION, 1.0);
                float3 _140 = _81[_123].xyz / float3(_81[_123].w);
                _81[_123] = float4(_140.x, _140.y, _140.z, _81[_123].w);
                _120 = ((((_120 > _123) && (abs(_81[_123].x) < 1.0)) && (abs(_81[_123].y) < 1.0)) && (abs(_81[_123].z) < 1.0)) ? _123 : _120;
                _123++;
                continue;
            }
            if (_120 == 4294967295u)
            {
                _352 = 1.0;
                break;
            }
            _81[_120].y *= (-1.0);
            float2 _168 = _81[_120].xy * 0.5;
            _81[_120] = float4(_168.x, _168.y, _81[_120].z, _81[_120].w);
            float2 _173 = _81[_120].xy + float2(0.5);
            _81[_120] = float4(_173.x, _173.y, _81[_120].z, _81[_120].w);
            _81[_120].w = float(_120);
            if (_120 < 3u)
            {
                float3 _324 = float3(_81[_120].xyw);
                float3 _332 = float3(_81[_120].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _340 = float3(_81[_120].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _347 = float3(_81[_120].xy + fragmentUniforms.directionalShadowInfo, _81[_120].w);
                _352 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _324.xy, uint(round(_324.z)), _81[_120].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _332.xy, uint(round(_332.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _340.xy, uint(round(_340.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _347.xy, uint(round(_347.z)), _81[_120].z, level(0.0))) * 0.25;
                break;
            }
            else
            {
                float3 _193 = float3(_81[_120].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _201 = float3(_81[_120].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _209 = float3(_81[_120].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _217 = float3(_81[_120].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _225 = float3(_81[_120].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _233 = float3(_81[_120].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _241 = float3(_81[_120].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _249 = float3(_81[_120].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _257 = float3(_81[_120].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _265 = float3(_81[_120].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _271 = float3(_81[_120].xyw);
                float3 _279 = float3(_81[_120].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _287 = float3(_81[_120].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _295 = float3(_81[_120].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _303 = float3(_81[_120].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _81[_120].w);
                float3 _310 = float3(_81[_120].xy + fragmentUniforms.directionalShadowInfo, _81[_120].w);
                _352 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _193.xy, uint(round(_193.z)), _81[_120].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _201.xy, uint(round(_201.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _209.xy, uint(round(_209.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _217.xy, uint(round(_217.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _225.xy, uint(round(_225.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _233.xy, uint(round(_233.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _241.xy, uint(round(_241.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _249.xy, uint(round(_249.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _257.xy, uint(round(_257.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _265.xy, uint(round(_265.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _271.xy, uint(round(_271.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _279.xy, uint(round(_279.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _287.xy, uint(round(_287.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _295.xy, uint(round(_295.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _303.xy, uint(round(_303.z)), _81[_120].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _310.xy, uint(round(_310.z)), _81[_120].z, level(0.0))) * 0.0625;
                break;
            }
        }
    }
    float4 _354 = _91;
    _354.w = 1.0;
    out.out_var_SV_TARGET = fragmentUniforms.diffuseColor * (fragmentUniforms.ambientColor + _354);
    return out;
}

