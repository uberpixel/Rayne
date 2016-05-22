//
//  RNGPUBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNGPUBuffer.h"

namespace RN
{
	RNDefineMeta(GPUBuffer, GPUResource)

	GPUBuffer::GPUBuffer()
	{}

	GPUBuffer::~GPUBuffer()
	{}

	void GPUBuffer::Invalidate()
	{
		InvalidateRange(Range(0, GetLength()));
	}
}
