//
//  RNRenderingDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderingDescriptor.h"

namespace RN
{
	RenderingDescriptor::RenderingDescriptor(const String *identifier, const String *api) :
		_identifier(identifier->Copy()),
		_api(api->Copy())
	{}

	RenderingDescriptor::~RenderingDescriptor()
	{
		_identifier->Release();
		_api->Release();
	}
}
