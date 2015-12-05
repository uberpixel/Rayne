#include <metal_stdlib>

using namespace metal;

struct Vertex
{
	float2 position [[attribute(0)]];
	float2 texCoords [[attribute(1)]];
};

struct VertexResult
{
	float4 position [[position]];
	float2 texCoords;
};

vertex VertexResult basic_blit_vertex(Vertex vert [[stage_in]])
{
	VertexResult result;
	result.position = float4(vert.position, 0.0, 1.0);
	result.texCoords = vert.texCoords;

	return result;
}

fragment float4 basic_blit_fragment(VertexResult vert [[stage_in]], texture2d<float> texture [[texture(0)]], sampler samplr [[sampler(0)]])
{
	return texture.sample(samplr, vert.texCoords).rgba;
}
