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
    float4 in_var_COLOR0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], depth2d_array<float> directionalShadowTexture [[texture(0)]], sampler directionalShadowSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _94;
    _94 = float4(0.0);
    float4 _110;
    float _355;
    for (uint _97 = 0u; _97 < fragmentUniforms.directionalLightsCount; _94 += (_110 * _355), _97++)
    {
        _110 = fragmentUniforms.directionalLights[_97].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_97].direction.xyz), 0.0, 1.0);
        for (;;)
        {
            if ((int(_97) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
            {
                _355 = 1.0;
                break;
            }
            uint _123;
            _123 = 4294967295u;
            float4 _82[4];
            for (uint _126 = 0u; _126 < fragmentUniforms.directionalShadowMatricesCount; )
            {
                _82[_126] = fragmentUniforms.directionalShadowMatrices[_126] * float4(in.in_var_POSITION, 1.0);
                float3 _143 = _82[_126].xyz / float3(_82[_126].w);
                _82[_126] = float4(_143.x, _143.y, _143.z, _82[_126].w);
                _123 = ((((_123 > _126) && (abs(_82[_126].x) < 1.0)) && (abs(_82[_126].y) < 1.0)) && (abs(_82[_126].z) < 1.0)) ? _126 : _123;
                _126++;
                continue;
            }
            if (_123 == 4294967295u)
            {
                _355 = 1.0;
                break;
            }
            _82[_123].y *= (-1.0);
            float2 _171 = _82[_123].xy * 0.5;
            _82[_123] = float4(_171.x, _171.y, _82[_123].z, _82[_123].w);
            float2 _176 = _82[_123].xy + float2(0.5);
            _82[_123] = float4(_176.x, _176.y, _82[_123].z, _82[_123].w);
            _82[_123].w = float(_123);
            if (_123 < 3u)
            {
                float3 _327 = float3(_82[_123].xyw);
                float3 _335 = float3(_82[_123].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _343 = float3(_82[_123].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _350 = float3(_82[_123].xy + fragmentUniforms.directionalShadowInfo, _82[_123].w);
                _355 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _327.xy, uint(round(_327.z)), _82[_123].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _335.xy, uint(round(_335.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _343.xy, uint(round(_343.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _350.xy, uint(round(_350.z)), _82[_123].z, level(0.0))) * 0.25;
                break;
            }
            else
            {
                float3 _196 = float3(_82[_123].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _204 = float3(_82[_123].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _212 = float3(_82[_123].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _220 = float3(_82[_123].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _228 = float3(_82[_123].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _236 = float3(_82[_123].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _244 = float3(_82[_123].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _252 = float3(_82[_123].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _260 = float3(_82[_123].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _268 = float3(_82[_123].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _274 = float3(_82[_123].xyw);
                float3 _282 = float3(_82[_123].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _290 = float3(_82[_123].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _298 = float3(_82[_123].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _306 = float3(_82[_123].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _82[_123].w);
                float3 _313 = float3(_82[_123].xy + fragmentUniforms.directionalShadowInfo, _82[_123].w);
                _355 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _196.xy, uint(round(_196.z)), _82[_123].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _204.xy, uint(round(_204.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _212.xy, uint(round(_212.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _220.xy, uint(round(_220.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _228.xy, uint(round(_228.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _236.xy, uint(round(_236.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _244.xy, uint(round(_244.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _252.xy, uint(round(_252.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _260.xy, uint(round(_260.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _268.xy, uint(round(_268.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _274.xy, uint(round(_274.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _282.xy, uint(round(_282.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _290.xy, uint(round(_290.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _298.xy, uint(round(_298.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _306.xy, uint(round(_306.z)), _82[_123].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _313.xy, uint(round(_313.z)), _82[_123].z, level(0.0))) * 0.0625;
                break;
            }
        }
    }
    float4 _357 = _94;
    _357.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _357);
    return out;
}

