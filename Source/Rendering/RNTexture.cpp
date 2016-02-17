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

	static uint32 _defaultAnisotropy = 1;

	Texture::Texture(const Descriptor &descriptor) :
		_descriptor(descriptor)
	{}

	Texture *Texture::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Texture>(name, settings);
	}

	void Texture::SetParameter(const Parameter &parameter)
	{
		_parameter = parameter;
	}

	const String *Texture::Descriptor::__TranslateFormat(Format format)
	{
		Renderer *renderer = Renderer::GetActiveRenderer();
		return renderer->GetTextureFormatName(format);
	}

	uint32 Texture::GetDefaultAnisotropy()
	{
		return _defaultAnisotropy;
	}
	void Texture::SetDefaultAnisotropy(uint32 anisotropy)
	{
		RN_ASSERT(anisotropy >= 1 && anisotropy <= 16, "Anisotropy must be [1, 16]");
		_defaultAnisotropy = anisotropy;
	}
}
