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
		_sampler(nullptr),
		_data(nullptr)
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
		// No error handling here so far
		if (_data != nullptr) {
			delete[] _data;
		}
		auto numBytes = _descriptor.height * bytesPerRow;
		_data = static_cast<char*>(new char[numBytes]);
		std::memcpy(_data, bytes, numBytes);
/*		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture replaceRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel withBytes:bytes bytesPerRow:bytesPerRow];*/
	}
	void D3D12Texture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow)
	{
		// slice is being ignored here so far
		SetData(region, mipmapLevel, bytes, bytesPerRow);	
/*		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture replaceRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel withBytes:bytes bytesPerRow:bytesPerRow];*/
	}
	void D3D12Texture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const 
	{
		// No error handling here either (so far)
		auto numBytes = _descriptor.height * bytesPerRow;
		std::memcpy(bytes, _data, numBytes);
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
