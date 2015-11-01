//
//  RendererDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRendererDescriptor.h"

namespace RN
{
	RNDefineMeta(RendererDescriptor, Object)

	RendererDescriptor::RendererDescriptor(const String *identifier, const String *api) :
		_identifier(identifier->Copy()),
		_api(api->Copy())
	{}

	RendererDescriptor::~RendererDescriptor()
	{
		_identifier->Release();
		_api->Release();
	}
}
