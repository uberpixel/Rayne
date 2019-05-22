#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_vertexUniforms
{
    float4x4 modelViewMatrix;
    float4x4 projectionMatrix;
};

struct depth_vertex_out
{
    float4 gl_Position [[position]];
};

struct depth_vertex_in
{
    float3 in_var_POSITION [[attribute(0)]];
};

vertex depth_vertex_out depth_vertex(depth_vertex_in in [[stage_in]], constant type_vertexUniforms& vertexUniforms [[buffer(1)]])
{
    depth_vertex_out out = {};
    out.gl_Position = (vertexUniforms.projectionMatrix * float4((vertexUniforms.modelViewMatrix * float4(in.in_var_POSITION, 1.0)).xyz, 1.0)).xyww;
    return out;
}

