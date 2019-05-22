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
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], depth2d_array<float> directionalShadowTexture [[texture(1)]], sampler linearRepeatSampler [[sampler(0)]], sampler directionalShadowSampler [[sampler(1)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _103;
    _103 = float4(0.0);
    float4 _119;
    float _364;
    for (uint _106 = 0u; _106 < fragmentUniforms.directionalLightsCount; _103 += (_119 * _364), _106++)
    {
        _119 = fragmentUniforms.directionalLights[_106].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_106].direction.xyz), 0.0, 1.0);
        for (;;)
        {
            if ((int(_106) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
            {
                _364 = 1.0;
                break;
            }
            uint _132;
            _132 = 4294967295u;
            float4 _87[4];
            for (uint _135 = 0u; _135 < fragmentUniforms.directionalShadowMatricesCount; )
            {
                _87[_135] = fragmentUniforms.directionalShadowMatrices[_135] * float4(in.in_var_POSITION, 1.0);
                float3 _152 = _87[_135].xyz / float3(_87[_135].w);
                _87[_135] = float4(_152.x, _152.y, _152.z, _87[_135].w);
                _132 = ((((_132 > _135) && (abs(_87[_135].x) < 1.0)) && (abs(_87[_135].y) < 1.0)) && (abs(_87[_135].z) < 1.0)) ? _135 : _132;
                _135++;
                continue;
            }
            if (_132 == 4294967295u)
            {
                _364 = 1.0;
                break;
            }
            _87[_132].y *= (-1.0);
            float2 _180 = _87[_132].xy * 0.5;
            _87[_132] = float4(_180.x, _180.y, _87[_132].z, _87[_132].w);
            float2 _185 = _87[_132].xy + float2(0.5);
            _87[_132] = float4(_185.x, _185.y, _87[_132].z, _87[_132].w);
            _87[_132].w = float(_132);
            if (_132 < 3u)
            {
                float3 _336 = float3(_87[_132].xyw);
                float3 _344 = float3(_87[_132].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _352 = float3(_87[_132].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _359 = float3(_87[_132].xy + fragmentUniforms.directionalShadowInfo, _87[_132].w);
                _364 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _336.xy, uint(round(_336.z)), _87[_132].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _344.xy, uint(round(_344.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _352.xy, uint(round(_352.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _359.xy, uint(round(_359.z)), _87[_132].z, level(0.0))) * 0.25;
                break;
            }
            else
            {
                float3 _205 = float3(_87[_132].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _213 = float3(_87[_132].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _221 = float3(_87[_132].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _229 = float3(_87[_132].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _237 = float3(_87[_132].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _245 = float3(_87[_132].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _253 = float3(_87[_132].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _261 = float3(_87[_132].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _269 = float3(_87[_132].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _277 = float3(_87[_132].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _283 = float3(_87[_132].xyw);
                float3 _291 = float3(_87[_132].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _299 = float3(_87[_132].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _307 = float3(_87[_132].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _315 = float3(_87[_132].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _87[_132].w);
                float3 _322 = float3(_87[_132].xy + fragmentUniforms.directionalShadowInfo, _87[_132].w);
                _364 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _205.xy, uint(round(_205.z)), _87[_132].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _213.xy, uint(round(_213.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _221.xy, uint(round(_221.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _229.xy, uint(round(_229.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _237.xy, uint(round(_237.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _245.xy, uint(round(_245.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _253.xy, uint(round(_253.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _261.xy, uint(round(_261.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _269.xy, uint(round(_269.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _277.xy, uint(round(_277.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _283.xy, uint(round(_283.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _291.xy, uint(round(_291.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _299.xy, uint(round(_299.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _307.xy, uint(round(_307.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _315.xy, uint(round(_315.z)), _87[_132].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _322.xy, uint(round(_322.z)), _87[_132].z, level(0.0))) * 0.0625;
                break;
            }
        }
    }
    float4 _366 = _103;
    _366.w = 1.0;
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0)) * (fragmentUniforms.ambientColor + _366);
    return out;
}

