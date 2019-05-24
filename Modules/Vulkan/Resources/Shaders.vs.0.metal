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
    float4 gl_Position [[position]];
};

struct gouraud_vertex_in
{
    float3 in_var_POSITION [[attribute(0)]];
};

vertex gouraud_vertex_out gouraud_vertex(gouraud_vertex_in in [[stage_in]], constant type_vertexUniforms& vertexUniforms [[buffer(1)]])
{
    gouraud_vertex_out out = {};
    float4 _29 = float4(in.in_var_POSITION, 1.0);
    out.gl_Position = vertexUniforms.modelViewProjectionMatrix * _29;
    out.out_var_POSITION = (vertexUniforms.modelMatrix * _29).xyz;
    return out;
}

