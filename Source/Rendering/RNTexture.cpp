//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Assets/RNAssetCoordinator.h"
#include "RNTexture.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(Texture, Asset)

	Texture::Texture(const Descriptor &descriptor) :
		_descriptor(descriptor)
	{}

	Texture *Texture::WithName(const String *name)
	{
		AssetCoordinator *coordinator = AssetCoordinator::GetSharedInstance();
		return coordinator->GetAssetWithName<Texture>(name, nullptr);
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
}
