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

fragment gouraud_fragment_out gouraud_fragment(constant type_fragmentUniforms& fragmentUniforms [[buffer(2)]], float4 gl_FragCoord [[position]])
{
    gouraud_fragment_out out = {};
    out.out_var_SV_TARGET = fragmentUniforms.diffuseColor * fragmentUniforms.ambientColor;
    return out;
}

