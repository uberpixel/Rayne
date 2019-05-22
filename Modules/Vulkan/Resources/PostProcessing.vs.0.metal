#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct pp_vertex_out
{
    float2 out_var_TEXCOORD0 [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct pp_vertex_in
{
    float3 in_var_POSITION [[attribute(0)]];
    float2 in_var_TEXCOORD0 [[attribute(1)]];
};

vertex pp_vertex_out pp_vertex(pp_vertex_in in [[stage_in]])
{
    pp_vertex_out out = {};
    out.gl_Position = float4(in.in_var_POSITION.xy, 0.0, 1.0);
    out.out_var_TEXCOORD0 = in.in_var_TEXCOORD0;
    return out;
}

