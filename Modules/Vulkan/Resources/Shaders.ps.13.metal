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
    float2 in_var_TEXCOORD0 [[user(locn2)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], depth2d_array<float> directionalShadowTexture [[texture(1)]], sampler linearRepeatSampler [[sampler(0)]], sampler directionalShadowSampler [[sampler(1)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _388;
    for (;;)
    {
        float4 _104 = fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0);
        float _110 = smoothstep(fragmentUniforms.alphaToCoverageClamp.x, fragmentUniforms.alphaToCoverageClamp.y, _104.w);
        float4 _111 = _104;
        _111.w = _110;
        if (_110 < 0.001000000047497451305389404296875)
        {
            _388 = _111;
            break;
        }
        float4 _120;
        _120 = float4(0.0);
        float4 _136;
        float _381;
        for (uint _123 = 0u; _123 < fragmentUniforms.directionalLightsCount; _120 += (_136 * _381), _123++)
        {
            _136 = fragmentUniforms.directionalLights[_123].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_123].direction.xyz), 0.0, 1.0);
            for (;;)
            {
                if ((int(_123) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
                {
                    _381 = 1.0;
                    break;
                }
                uint _149;
                _149 = 4294967295u;
                float4 _90[4];
                for (uint _152 = 0u; _152 < fragmentUniforms.directionalShadowMatricesCount; )
                {
                    _90[_152] = fragmentUniforms.directionalShadowMatrices[_152] * float4(in.in_var_POSITION, 1.0);
                    float3 _169 = _90[_152].xyz / float3(_90[_152].w);
                    _90[_152] = float4(_169.x, _169.y, _169.z, _90[_152].w);
                    _149 = ((((_149 > _152) && (abs(_90[_152].x) < 1.0)) && (abs(_90[_152].y) < 1.0)) && (abs(_90[_152].z) < 1.0)) ? _152 : _149;
                    _152++;
                    continue;
                }
                if (_149 == 4294967295u)
                {
                    _381 = 1.0;
                    break;
                }
                _90[_149].y *= (-1.0);
                float2 _197 = _90[_149].xy * 0.5;
                _90[_149] = float4(_197.x, _197.y, _90[_149].z, _90[_149].w);
                float2 _202 = _90[_149].xy + float2(0.5);
                _90[_149] = float4(_202.x, _202.y, _90[_149].z, _90[_149].w);
                _90[_149].w = float(_149);
                if (_149 < 3u)
                {
                    float3 _353 = float3(_90[_149].xyw);
                    float3 _361 = float3(_90[_149].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _369 = float3(_90[_149].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _376 = float3(_90[_149].xy + fragmentUniforms.directionalShadowInfo, _90[_149].w);
                    _381 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _353.xy, uint(round(_353.z)), _90[_149].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _361.xy, uint(round(_361.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _369.xy, uint(round(_369.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _376.xy, uint(round(_376.z)), _90[_149].z, level(0.0))) * 0.25;
                    break;
                }
                else
                {
                    float3 _222 = float3(_90[_149].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _230 = float3(_90[_149].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _238 = float3(_90[_149].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _246 = float3(_90[_149].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _254 = float3(_90[_149].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _262 = float3(_90[_149].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _270 = float3(_90[_149].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _278 = float3(_90[_149].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _286 = float3(_90[_149].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _294 = float3(_90[_149].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _300 = float3(_90[_149].xyw);
                    float3 _308 = float3(_90[_149].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _316 = float3(_90[_149].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _324 = float3(_90[_149].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _332 = float3(_90[_149].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _90[_149].w);
                    float3 _339 = float3(_90[_149].xy + fragmentUniforms.directionalShadowInfo, _90[_149].w);
                    _381 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _222.xy, uint(round(_222.z)), _90[_149].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _230.xy, uint(round(_230.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _238.xy, uint(round(_238.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _246.xy, uint(round(_246.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _254.xy, uint(round(_254.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _262.xy, uint(round(_262.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _270.xy, uint(round(_270.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _278.xy, uint(round(_278.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _286.xy, uint(round(_286.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _294.xy, uint(round(_294.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _300.xy, uint(round(_300.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _308.xy, uint(round(_308.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _316.xy, uint(round(_316.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _324.xy, uint(round(_324.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _332.xy, uint(round(_332.z)), _90[_149].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _339.xy, uint(round(_339.z)), _90[_149].z, level(0.0))) * 0.0625;
                    break;
                }
            }
        }
        float4 _383 = _120;
        _383.w = 1.0;
        _388 = _111 * (fragmentUniforms.ambientColor + _383);
        break;
    }
    out.out_var_SV_TARGET = _388;
    return out;
}

