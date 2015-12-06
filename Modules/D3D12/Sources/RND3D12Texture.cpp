//
//  RND3D12Texture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Texture.h"
#include "RND3D12Renderer.h"
#include "RND3D12StateCoordinator.h"

namespace RN
{
	RNDefineMeta(D3D12Texture, Texture)

	D3D12Texture::D3D12Texture(D3D12Renderer *renderer, D3D12StateCoordinator *coordinator, void *texture, const Descriptor &descriptor) :
		Texture(descriptor),
		_renderer(renderer),
		_coordinator(coordinator),
		_texture(texture),
		_sampler(nullptr)
	{
		SetParameter(GetParameter());
	}

	D3D12Texture::~D3D12Texture()
	{
/*		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture release];

		id<MTLSamplerState> sampler = (id<MTLSamplerState>)_sampler;
		[sampler release];*/
	}

	void D3D12Texture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		SetData(Region::With2D(0, 0, _descriptor.width, _descriptor.height), mipmapLevel, bytes, bytesPerRow);
	}
	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
/*		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture replaceRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel withBytes:bytes bytesPerRow:bytesPerRow];*/
	}
	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow)
	{
/*		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture replaceRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel withBytes:bytes bytesPerRow:bytesPerRow];*/
	}

	void D3D12Texture::GenerateMipMaps()
	{
		_renderer->CreateMipMapForeTexture(this);
	}

	void D3D12Texture::SetParameter(const Parameter &parameter)
	{
		Texture::SetParameter(parameter);

/*		id<MTLSamplerState> sampler = (id<MTLSamplerState>)_sampler;
		[sampler release];

		sampler = [_coordinator->GetSamplerStateForTextureParameter(parameter) retain];
		_sampler = sampler;*/
	}

	bool D3D12Texture::HasColorChannel(ColorChannel channel) const
	{
		return false;
	}
}
