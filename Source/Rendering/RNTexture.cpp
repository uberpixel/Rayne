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
			ColorChannel(Format::RGBA_8_SRGB, true, true, true, true)
			ColorChannel(Format::BGRA_8_SRGB, true, true, true, true)
				
			ColorChannel(Format::RGB_8_SRGB, true, true, true, false)
			ColorChannel(Format::BGR_8_SRGB, true, true, true, false)
				
			ColorChannel(Format::RGBA_8, true, true, true, true)
			ColorChannel(Format::BGRA_8, true, true, true, true)
			ColorChannel(Format::RGB_10_A_2, true, true, true, true)
				
			ColorChannel(Format::R_8, true, false, false, false)
			ColorChannel(Format::RG_8, true, true, false, false)
			ColorChannel(Format::RGB_8, true, true, true, false)
				
			ColorChannel(Format::R_16F, true, false, false, false)
			ColorChannel(Format::RG_16F, true, true, false, false)
			ColorChannel(Format::RGB_16F, true, true, true, false)
			ColorChannel(Format::RGBA_16F, true, true, true, true)
				
			ColorChannel(Format::R_32F, true, false, false, false)
			ColorChannel(Format::RG_32F, true, true, false, false)
			ColorChannel(Format::RGB_32F, true, true, true, false)
			ColorChannel(Format::RGBA_32F, true, true, true, true)
				
			default:
				return false;
		}
	}
}
