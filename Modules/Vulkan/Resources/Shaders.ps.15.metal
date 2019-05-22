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
    float2 in_var_TEXCOORD0 [[user(locn3)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], depth2d_array<float> directionalShadowTexture [[texture(1)]], sampler linearRepeatSampler [[sampler(0)]], sampler directionalShadowSampler [[sampler(1)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _391;
    for (;;)
    {
        float4 _106 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _112 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _106.w);
        float4 _113 = _106;
        _113.w = _112;
        if (_112 < 0.001000000047497451305389404296875)
        {
            _391 = _113;
            break;
        }
        float4 _123;
        _123 = float4(0.0);
        float4 _139;
        float _384;
        for (uint _126 = 0u; _126 < fragmentUniforms.directionalLightsCount; _123 += (_139 * _384), _126++)
        {
            _139 = fragmentUniforms.directionalLights[_126].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_126].direction.xyz), 0.0, 1.0);
            for (;;)
            {
                if ((int(_126) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
                {
                    _384 = 1.0;
                    break;
                }
                uint _152;
                _152 = 4294967295u;
                float4 _91[4];
                for (uint _155 = 0u; _155 < fragmentUniforms.directionalShadowMatricesCount; )
                {
                    _91[_155] = fragmentUniforms.directionalShadowMatrices[_155] * float4(in.in_var_POSITION, 1.0);
                    float3 _172 = _91[_155].xyz / float3(_91[_155].w);
                    _91[_155] = float4(_172.x, _172.y, _172.z, _91[_155].w);
                    _152 = ((((_152 > _155) && (abs(_91[_155].x) < 1.0)) && (abs(_91[_155].y) < 1.0)) && (abs(_91[_155].z) < 1.0)) ? _155 : _152;
                    _155++;
                    continue;
                }
                if (_152 == 4294967295u)
                {
                    _384 = 1.0;
                    break;
                }
                _91[_152].y *= (-1.0);
                float2 _200 = _91[_152].xy * 0.5;
                _91[_152] = float4(_200.x, _200.y, _91[_152].z, _91[_152].w);
                float2 _205 = _91[_152].xy + float2(0.5);
                _91[_152] = float4(_205.x, _205.y, _91[_152].z, _91[_152].w);
                _91[_152].w = float(_152);
                if (_152 < 3u)
                {
                    float3 _356 = float3(_91[_152].xyw);
                    float3 _364 = float3(_91[_152].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _372 = float3(_91[_152].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _379 = float3(_91[_152].xy + fragmentUniforms.directionalShadowInfo, _91[_152].w);
                    _384 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _356.xy, uint(round(_356.z)), _91[_152].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _364.xy, uint(round(_364.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _372.xy, uint(round(_372.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _379.xy, uint(round(_379.z)), _91[_152].z, level(0.0))) * 0.25;
                    break;
                }
                else
                {
                    float3 _225 = float3(_91[_152].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _233 = float3(_91[_152].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _241 = float3(_91[_152].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _249 = float3(_91[_152].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _257 = float3(_91[_152].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _265 = float3(_91[_152].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _273 = float3(_91[_152].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _281 = float3(_91[_152].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _289 = float3(_91[_152].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _297 = float3(_91[_152].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _303 = float3(_91[_152].xyw);
                    float3 _311 = float3(_91[_152].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _319 = float3(_91[_152].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _327 = float3(_91[_152].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _335 = float3(_91[_152].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _91[_152].w);
                    float3 _342 = float3(_91[_152].xy + fragmentUniforms.directionalShadowInfo, _91[_152].w);
                    _384 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _225.xy, uint(round(_225.z)), _91[_152].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _233.xy, uint(round(_233.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _241.xy, uint(round(_241.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _249.xy, uint(round(_249.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _257.xy, uint(round(_257.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _265.xy, uint(round(_265.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _273.xy, uint(round(_273.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _281.xy, uint(round(_281.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _289.xy, uint(round(_289.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _297.xy, uint(round(_297.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _303.xy, uint(round(_303.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _311.xy, uint(round(_311.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _319.xy, uint(round(_319.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _327.xy, uint(round(_327.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _335.xy, uint(round(_335.z)), _91[_152].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _342.xy, uint(round(_342.z)), _91[_152].z, level(0.0))) * 0.0625;
                    break;
                }
            }
        }
        float4 _386 = _123;
        _386.w = 1.0;
        _391 = (_113 * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _386);
        break;
    }
    out.out_var_SV_TARGET = _391;
    return out;
}

