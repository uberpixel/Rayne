
Texture2D<float4> SrcTexture : register(t0);
RWTexture2D<float4> DstTexture : register(u0);
SamplerState BilinearClamp : register(s0);


cbuffer CB : register(b0)
{
	float2 TexelSize;	// 1.0 / OutMip1.Dimensions
}

[numthreads( 8, 8, 1 )]
void GenerateMipMaps(uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID)
{
	float2 texcoords = TexelSize * (DTid.xy + 0.5);
	float4 color = SrcTexture.SampleLevel(BilinearClamp, texcoords, 0);
	DstTexture[DTid.xy] = color;
}
