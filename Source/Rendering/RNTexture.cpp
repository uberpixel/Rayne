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
}
