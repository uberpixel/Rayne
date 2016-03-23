//
//  RNDynamicGPUBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDynamicGPUBuffer.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(DynamicGPUBuffer, Object)

	DynamicGPUBuffer::DynamicGPUBuffer(size_t initial, GPUResource::UsageOptions usageOption) :
		_index(0),
		_length(std::max(static_cast<size_t>(5), initial)),
		_usageOption(usageOption)
	{
		Renderer *renderer = Renderer::GetActiveRenderer();

		for(size_t i = 0; i < 3; i ++)
			_buffers[i] = renderer->CreateBufferWithLength(_length, usageOption, GPUResource::AccessOptions::ReadWrite);
	}

	DynamicGPUBuffer::~DynamicGPUBuffer()
	{
		for(size_t i = 0; i < 3; i ++)
			SafeRelease(_buffers[i]);
	}

	void DynamicGPUBuffer::Resize(size_t length)
	{
		_length = length;
	}

	void DynamicGPUBuffer::Advance()
	{
		_index = (_index + 1) % 3;

		GPUBuffer *buffer = _buffers[_index];

		if(buffer->GetLength() < _length)
		{
			buffer->Release();
			_buffers[_index] = Renderer::GetActiveRenderer()->CreateBufferWithLength(_length, _usageOption, GPUBuffer::AccessOptions::ReadWrite);
		}
		else if(buffer->GetLength() / 2 > _length)
		{
			buffer->Release();
			_buffers[_index] = Renderer::GetActiveRenderer()->CreateBufferWithLength(_length, _usageOption, GPUBuffer::AccessOptions::ReadWrite);
		}
	}

	void *DynamicGPUBuffer::GetBuffer()
	{
		return _buffers[_index]->GetBuffer();
	}
	void DynamicGPUBuffer::Invalidate()
	{
		_buffers[_index]->Invalidate();
	}
	void DynamicGPUBuffer::InvalidateRange(const Range &range)
	{
		_buffers[_index]->InvalidateRange(range);
	}
}
