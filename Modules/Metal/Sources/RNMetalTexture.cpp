//
//  RNMetalTexture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalTexture.h"
#include "RNMetalRenderer.h"
#include "RNMetalStateCoordinator.h"

namespace RN
{
	RNDefineMeta(MetalTexture, Texture)

	MetalTexture::MetalTexture(MetalRenderer *renderer, MetalStateCoordinator *coordinator, void *texture, const Descriptor &descriptor) :
		Texture(descriptor),
		_renderer(renderer),
		_coordinator(coordinator),
		_texture(texture),
		_sampler(nullptr)
	{
		SetParameter(GetParameter());
	}

	MetalTexture::~MetalTexture()
	{
		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture release];

		id<MTLSamplerState> sampler = (id<MTLSamplerState>)_sampler;
		[sampler release];
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

	void MetalTexture::GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const
	{
		Region region = Region::With2D(0, 0, _descriptor.width, _descriptor.height);

		id<MTLTexture> texture = (id<MTLTexture>)_texture;
		[texture getBytes:bytes bytesPerRow:bytesPerRow fromRegion:MTLRegionMake3D(region.x, region.y, region.z, region.width, region.height, region.depth) mipmapLevel:mipmapLevel];
	}

	void MetalTexture::GenerateMipMaps()
	{
		_renderer->CreateMipMapForTexture(this);
	}

	void MetalTexture::SetParameter(const Parameter &parameter)
	{
		Texture::SetParameter(parameter);

		id<MTLSamplerState> sampler = (id<MTLSamplerState>)_sampler;
		[sampler release];

		sampler = [_coordinator->GetSamplerStateForTextureParameter(parameter) retain];
		_sampler = sampler;
	}

	bool MetalTexture::HasColorChannel(ColorChannel channel) const
	{
#define ColorChannel(format, r, g, b, a) \
		case format: \
		{ \
			switch(channel) \
			{ \
				case ColorChannel::Red: \
					return r; \
				case ColorChannel::Green: \
					return g; \
				case ColorChannel::Blue: \
					return b; \
				case ColorChannel::Alpha: \
					return a; \
			} \
			return false; \
		}

		switch([(id <MTLTexture>)_texture pixelFormat])
		{
			ColorChannel(MTLPixelFormatRGBA8Unorm, true, true, true, true)
			ColorChannel(MTLPixelFormatRGB10A2Unorm, true, true, true, true)
			ColorChannel(MTLPixelFormatR8Unorm, true, false, false, false)
			ColorChannel(MTLPixelFormatRG8Unorm, true, true, false, false)

			ColorChannel(MTLPixelFormatR16Float, true, false, false, false)
			ColorChannel(MTLPixelFormatRG16Float, true, true, false, false)
			ColorChannel(MTLPixelFormatRGBA16Float, true, true, true, true)

			ColorChannel(MTLPixelFormatR32Float, true, false, false, false)
			ColorChannel(MTLPixelFormatRG32Float, true, true, false, false)
			ColorChannel(MTLPixelFormatRGBA32Float, true, true, true, true)

			ColorChannel(MTLPixelFormatDepth32Float, false, false, false, false)
			ColorChannel(MTLPixelFormatStencil8, false, false, false, false)
			ColorChannel(MTLPixelFormatDepth24Unorm_Stencil8, false, false, false, false)
			ColorChannel(MTLPixelFormatDepth32Float_Stencil8, false, false, false, false)

			default:
				return false;
		}
	}

	MTLPixelFormat MetalTexture::PixelFormatForTextureFormat(Format format)
	{
		switch(format)
		{
			case Format::RGBA8888:
				return MTLPixelFormatRGBA8Unorm;
			case Format::RGB10A2:
				return MTLPixelFormatRGB10A2Unorm;
			case Format::R8:
				return MTLPixelFormatR8Unorm;
			case Format::RG88:
				return MTLPixelFormatRG8Unorm;
			case Format::R16F:
				return MTLPixelFormatR16Float;
			case Format::RG16F:
				return MTLPixelFormatRG16Float;
			case Format::RGBA16F:
				return MTLPixelFormatRGBA16Float;
			case Format::R32F:
				return MTLPixelFormatR32Float;
			case Format::RG32F:
				return MTLPixelFormatRG32Float;
			case Format::RGBA32F:
				return MTLPixelFormatRGBA32Float;
			case Format::Depth32F:
				return MTLPixelFormatDepth32Float;
			case Format::Stencil8:
				return MTLPixelFormatStencil8;
			case Format::Depth24I:
				return MTLPixelFormatDepth24Unorm_Stencil8;
			case Format::Depth32FStencil8:
				return MTLPixelFormatDepth32Float_Stencil8;
			case Format::Depth24Stencil8:
				return MTLPixelFormatDepth24Unorm_Stencil8;
		}
	}
}
