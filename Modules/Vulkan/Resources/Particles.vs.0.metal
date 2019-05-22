#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct type_vertexUniforms
{
    float4x4 inverseViewMatrix;
    float4x4 modelViewProjectionMatrix;
};

struct particles_vertex_out
{
    float4 out_var_COLOR [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct particles_vertex_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float4 in_var_COLOR [[attribute(3)]];
    float2 in_var_TEXCOORD1 [[attribute(6)]];
};

vertex particles_vertex_out particles_vertex(particles_vertex_in in [[stage_in]], constant type_vertexUniforms& vertexUniforms [[buffer(1)]])
{
    particles_vertex_out out = {};
    out.gl_Position = vertexUniforms.modelViewProjectionMatrix * (float4(in.in_var_POSITION, 1.0) + (vertexUniforms.inverseViewMatrix * float4(in.in_var_TEXCOORD1, 0.0, 0.0)));
    out.out_var_COLOR = in.in_var_COLOR;
    return out;
}

