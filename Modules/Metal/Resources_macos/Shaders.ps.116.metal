#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_fragmentUniforms
{
    float4 ambientColor;
    float4 diffuseColor;
    float4 cameraAmbientColor;
};

struct gouraud_fragment_out
{
    float4 out_var_SV_TARGET [[color(0)]];
};

fragment gouraud_fragment_out gouraud_fragment(constant type_fragmentUniforms& fragmentUniforms [[buffer(0)]])
{
    gouraud_fragment_out out = {};
    out.out_var_SV_TARGET = (fragmentUniforms.diffuseColor * fragmentUniforms.ambientColor) * fragmentUniforms.cameraAmbientColor;
    return out;
}

