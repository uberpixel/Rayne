//
//  RNMetalTexture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../../Base/RNBaseInternal.h"
#include "RNMetalTexture.h"

namespace RN
{
	RNDefineMeta(MetalTexture, Texture)

	MetalTexture::MetalTexture(void *texture, const Descriptor &descriptor) :
		Texture(descriptor),
		_texture(texture)
	{}

	MetalTexture::~MetalTexture()
	{
		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture release];
	}

	void MetalTexture::SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		SetData(Region::With2D(0, 0, _descriptor.width, _descriptor.height), mipmapLevel, bytes, bytesPerRow);
	}
	void MetalTexture::SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow)
	{
		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture replaceRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel withBytes:bytes bytesPerRow:bytesPerRow];
	}
	void MetalTexture::SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow)
	{
		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture replaceRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel withBytes:bytes bytesPerRow:bytesPerRow];
	}

	void MetalTexture::SetGenerateMipMaps()
	{}
}
