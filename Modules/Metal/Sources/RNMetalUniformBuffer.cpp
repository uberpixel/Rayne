//
//  RNMetalUniformBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalUniformBuffer.h"
#include "RNMetalInternals.h"
#include "RNMetalRenderer.h"

namespace RN
{
	RNDefineMeta(MetalUniformBuffer, Object)

	MetalUniformBuffer::MetalUniformBuffer(Renderer *renderer, size_t size, uint32 index) :
		_index(index),
		_bufferIndex(0)
	{
		//This seems to be working as alignement, but could cause problems with arrays of structs in the buffer
		if((size % 16) > 0)
			size += 16 - (size % 16);
		
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i++)
			_buffers[i] = renderer->CreateBufferWithLength(size, GPUResource::UsageOptions::Uniform , GPUResource::AccessOptions::WriteOnly);
	}

	MetalUniformBuffer::~MetalUniformBuffer()
	{
		for(size_t i = 0; i < kRNMetalUniformBufferCount; i++)
			_buffers[i]->Release();
	}

	GPUBuffer *MetalUniformBuffer::Advance()
	{
		_bufferIndex = (_bufferIndex + 1) % kRNMetalUniformBufferCount;
		return _buffers[_bufferIndex];
	}
}
