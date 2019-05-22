#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_vertexUniforms
{
    float4x4 modelViewProjectionMatrix;
    float4x4 modelMatrix;
};

struct gouraud_vertex_out
{
    float3 out_var_POSITION [[user(locn0)]];
    float3 out_var_NORMAL [[user(locn1)]];
    float4 gl_Position [[position]];
};

struct gouraud_vertex_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float3 in_var_NORMAL [[attribute(1)]];
};

vertex gouraud_vertex_out gouraud_vertex(gouraud_vertex_in in [[stage_in]], constant type_vertexUniforms& vertexUniforms [[buffer(1)]])
{
    gouraud_vertex_out out = {};
    float4 _33 = float4(in.in_var_POSITION, 1.0);
    out.gl_Position = vertexUniforms.modelViewProjectionMatrix * _33;
    out.out_var_POSITION = (vertexUniforms.modelMatrix * _33).xyz;
    out.out_var_NORMAL = (vertexUniforms.modelMatrix * float4(in.in_var_NORMAL, 0.0)).xyz;
    return out;
}

