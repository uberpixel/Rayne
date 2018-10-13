//
//  RNFramebuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFramebuffer.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(Framebuffer, Object)

	Framebuffer::Framebuffer(const Vector2 &size) :
		_size(size)
	{}
	Framebuffer::~Framebuffer()
	{}
}
