//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Assets/RNAssetCoordinator.h"
#include "RNTexture.h"

namespace RN
{
	RNDefineMeta(Texture, GPUResource)

	Texture::Texture(const Descriptor &descriptor) :
		_descriptor(descriptor)
	{}

	Texture *Texture::WithName(const String *name)
	{
		AssetCoordinator *coordinator = AssetCoordinator::GetSharedInstance();
		return coordinator->GetAssetWithName<Texture>(name, nullptr);
	}
}
