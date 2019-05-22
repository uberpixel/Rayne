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
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], depth2d_array<float> directionalShadowTexture [[texture(0)]], sampler directionalShadowSampler [[sampler(0)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _90;
    _90 = float4(0.0);
    float4 _106;
    float _351;
    for (uint _93 = 0u; _93 < fragmentUniforms.directionalLightsCount; _90 += (_106 * _351), _93++)
    {
        _106 = fragmentUniforms.directionalLights[_93].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_93].direction.xyz), 0.0, 1.0);
        for (;;)
        {
            if ((int(_93) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
            {
                _351 = 1.0;
                break;
            }
            uint _119;
            _119 = 4294967295u;
            float4 _80[4];
            for (uint _122 = 0u; _122 < fragmentUniforms.directionalShadowMatricesCount; )
            {
                _80[_122] = fragmentUniforms.directionalShadowMatrices[_122] * float4(in.in_var_POSITION, 1.0);
                float3 _139 = _80[_122].xyz / float3(_80[_122].w);
                _80[_122] = float4(_139.x, _139.y, _139.z, _80[_122].w);
                _119 = ((((_119 > _122) && (abs(_80[_122].x) < 1.0)) && (abs(_80[_122].y) < 1.0)) && (abs(_80[_122].z) < 1.0)) ? _122 : _119;
                _122++;
                continue;
            }
            if (_119 == 4294967295u)
            {
                _351 = 1.0;
                break;
            }
            _80[_119].y *= (-1.0);
            float2 _167 = _80[_119].xy * 0.5;
            _80[_119] = float4(_167.x, _167.y, _80[_119].z, _80[_119].w);
            float2 _172 = _80[_119].xy + float2(0.5);
            _80[_119] = float4(_172.x, _172.y, _80[_119].z, _80[_119].w);
            _80[_119].w = float(_119);
            if (_119 < 3u)
            {
                float3 _323 = float3(_80[_119].xyw);
                float3 _331 = float3(_80[_119].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _339 = float3(_80[_119].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _346 = float3(_80[_119].xy + fragmentUniforms.directionalShadowInfo, _80[_119].w);
                _351 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _323.xy, uint(round(_323.z)), _80[_119].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _331.xy, uint(round(_331.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _339.xy, uint(round(_339.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _346.xy, uint(round(_346.z)), _80[_119].z, level(0.0))) * 0.25;
                break;
            }
            else
            {
                float3 _192 = float3(_80[_119].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _200 = float3(_80[_119].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _208 = float3(_80[_119].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _216 = float3(_80[_119].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _224 = float3(_80[_119].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _232 = float3(_80[_119].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _240 = float3(_80[_119].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _248 = float3(_80[_119].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _256 = float3(_80[_119].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _264 = float3(_80[_119].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _270 = float3(_80[_119].xyw);
                float3 _278 = float3(_80[_119].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _286 = float3(_80[_119].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _294 = float3(_80[_119].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _302 = float3(_80[_119].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _80[_119].w);
                float3 _309 = float3(_80[_119].xy + fragmentUniforms.directionalShadowInfo, _80[_119].w);
                _351 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _192.xy, uint(round(_192.z)), _80[_119].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _200.xy, uint(round(_200.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _208.xy, uint(round(_208.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _216.xy, uint(round(_216.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _224.xy, uint(round(_224.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _232.xy, uint(round(_232.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _240.xy, uint(round(_240.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _248.xy, uint(round(_248.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _256.xy, uint(round(_256.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _264.xy, uint(round(_264.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _270.xy, uint(round(_270.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _278.xy, uint(round(_278.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _286.xy, uint(round(_286.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _294.xy, uint(round(_294.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _302.xy, uint(round(_302.z)), _80[_119].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _309.xy, uint(round(_309.z)), _80[_119].z, level(0.0))) * 0.0625;
                break;
            }
        }
    }
    float4 _353 = _90;
    _353.w = 1.0;
    out.out_var_SV_TARGET = fragmentUniforms.diffuseColor * (fragmentUniforms.ambientColor + _353);
    return out;
}

