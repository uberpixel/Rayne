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
    float2 in_var_TEXCOORD0 [[user(locn3)]];
};

fragment gouraud_fragment_out gouraud_fragment(gouraud_fragment_in in [[stage_in]], constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], texture2d<float> texture0 [[texture(0)]], depth2d_array<float> directionalShadowTexture [[texture(1)]], sampler linearRepeatSampler [[sampler(0)]], sampler directionalShadowSampler [[sampler(1)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    float4 _106;
    _106 = float4(0.0);
    float4 _122;
    float _367;
    for (uint _109 = 0u; _109 < fragmentUniforms.directionalLightsCount; _106 += (_122 * _367), _109++)
    {
        _122 = fragmentUniforms.directionalLights[_109].color * fast::clamp(dot(normalize(in.in_var_NORMAL), -fragmentUniforms.directionalLights[_109].direction.xyz), 0.0, 1.0);
        for (;;)
        {
            if ((int(_109) != 0) || (fragmentUniforms.directionalShadowMatricesCount == 0u))
            {
                _367 = 1.0;
                break;
            }
            uint _135;
            _135 = 4294967295u;
            float4 _88[4];
            for (uint _138 = 0u; _138 < fragmentUniforms.directionalShadowMatricesCount; )
            {
                _88[_138] = fragmentUniforms.directionalShadowMatrices[_138] * float4(in.in_var_POSITION, 1.0);
                float3 _155 = _88[_138].xyz / float3(_88[_138].w);
                _88[_138] = float4(_155.x, _155.y, _155.z, _88[_138].w);
                _135 = ((((_135 > _138) && (abs(_88[_138].x) < 1.0)) && (abs(_88[_138].y) < 1.0)) && (abs(_88[_138].z) < 1.0)) ? _138 : _135;
                _138++;
                continue;
            }
            if (_135 == 4294967295u)
            {
                _367 = 1.0;
                break;
            }
            _88[_135].y *= (-1.0);
            float2 _183 = _88[_135].xy * 0.5;
            _88[_135] = float4(_183.x, _183.y, _88[_135].z, _88[_135].w);
            float2 _188 = _88[_135].xy + float2(0.5);
            _88[_135] = float4(_188.x, _188.y, _88[_135].z, _88[_135].w);
            _88[_135].w = float(_135);
            if (_135 < 3u)
            {
                float3 _339 = float3(_88[_135].xyw);
                float3 _347 = float3(_88[_135].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _355 = float3(_88[_135].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _362 = float3(_88[_135].xy + fragmentUniforms.directionalShadowInfo, _88[_135].w);
                _367 = (((directionalShadowTexture.sample_compare(directionalShadowSampler, _339.xy, uint(round(_339.z)), _88[_135].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _347.xy, uint(round(_347.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _355.xy, uint(round(_355.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _362.xy, uint(round(_362.z)), _88[_135].z, level(0.0))) * 0.25;
                break;
            }
            else
            {
                float3 _208 = float3(_88[_135].xy + (float2(-2.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _216 = float3(_88[_135].xy + (float2(-1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _224 = float3(_88[_135].xy + (float2(0.0, -2.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _232 = float3(_88[_135].xy + (float2(1.0, -2.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _240 = float3(_88[_135].xy + (float2(-2.0, -1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _248 = float3(_88[_135].xy + (float2(-1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _256 = float3(_88[_135].xy + (float2(0.0, -1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _264 = float3(_88[_135].xy + (float2(1.0, -1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _272 = float3(_88[_135].xy + (float2(-2.0, 0.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _280 = float3(_88[_135].xy + (float2(-1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _286 = float3(_88[_135].xyw);
                float3 _294 = float3(_88[_135].xy + (float2(1.0, 0.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _302 = float3(_88[_135].xy + (float2(-2.0, 1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _310 = float3(_88[_135].xy + (float2(-1.0, 1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _318 = float3(_88[_135].xy + (float2(0.0, 1.0) * fragmentUniforms.directionalShadowInfo), _88[_135].w);
                float3 _325 = float3(_88[_135].xy + fragmentUniforms.directionalShadowInfo, _88[_135].w);
                _367 = (((((((((((((((directionalShadowTexture.sample_compare(directionalShadowSampler, _208.xy, uint(round(_208.z)), _88[_135].z, level(0.0)) + directionalShadowTexture.sample_compare(directionalShadowSampler, _216.xy, uint(round(_216.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _224.xy, uint(round(_224.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _232.xy, uint(round(_232.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _240.xy, uint(round(_240.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _248.xy, uint(round(_248.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _256.xy, uint(round(_256.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _264.xy, uint(round(_264.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _272.xy, uint(round(_272.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _280.xy, uint(round(_280.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _286.xy, uint(round(_286.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _294.xy, uint(round(_294.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _302.xy, uint(round(_302.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _310.xy, uint(round(_310.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _318.xy, uint(round(_318.z)), _88[_135].z, level(0.0))) + directionalShadowTexture.sample_compare(directionalShadowSampler, _325.xy, uint(round(_325.z)), _88[_135].z, level(0.0))) * 0.0625;
                break;
            }
        }
    }
    float4 _369 = _106;
    _369.w = 1.0;
    out.out_var_SV_TARGET = ((fragmentUniforms.diffuseColor * texture0.sample(linearRepeatSampler, in.in_var_TEXCOORD0)) * in.in_var_COLOR0) * (fragmentUniforms.ambientColor + _369);
    return out;
}

