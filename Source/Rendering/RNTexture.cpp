//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Assets/RNAssetManager.h"
#include "RNTexture.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(Texture, Asset)

	RNExceptionImp(InvalidTextureFormat)

	Texture::Texture(const Descriptor &descriptor) :
		_descriptor(descriptor)
	{}

	Texture::~Texture()
	{}

	Texture *Texture::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Texture>(name, settings);
	}

	Texture *Texture::WithDescriptor(const Descriptor &descriptor)
	{
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
		return texture->Autorelease();
	}
	
	bool Texture::HasColorChannel(ColorChannel channel) const
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
		
		switch(_descriptor.format)
		{
			ColorChannel(Format::RGBA8888SRGB, true, true, true, true)
			ColorChannel(Format::BGRA8888SRGB, true, true, true, true)
				
			ColorChannel(Format::RGB888SRGB, true, true, true, false)
			ColorChannel(Format::BGR888SRGB, true, true, true, false)
				
			ColorChannel(Format::RGBA8888, true, true, true, true)
			ColorChannel(Format::BGRA8888, true, true, true, true)
			ColorChannel(Format::RGB10A2, true, true, true, true)
				
			ColorChannel(Format::R8, true, false, false, false)
			ColorChannel(Format::RG88, true, true, false, false)
			ColorChannel(Format::RGB888, true, true, true, false)
				
			ColorChannel(Format::R16F, true, false, false, false)
			ColorChannel(Format::RG16F, true, true, false, false)
			ColorChannel(Format::RGB16F, true, true, true, false)
			ColorChannel(Format::RGBA16F, true, true, true, true)
				
			ColorChannel(Format::R32F, true, false, false, false)
			ColorChannel(Format::RG32F, true, true, false, false)
			ColorChannel(Format::RGB32F, true, true, true, false)
			ColorChannel(Format::RGBA32F, true, true, true, true)
				
			default:
				return false;
		}
	}
}
