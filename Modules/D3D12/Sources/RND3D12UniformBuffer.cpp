//
//  RND3D12UniformBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12UniformBuffer.h"
#include "RND3D12Renderer.h"
#include "RND3D12GPUBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12UniformBuffer, Object)

	D3D12UniformBuffer::D3D12UniformBuffer(Renderer *renderer, size_t size) :
		_bufferIndex(0)
	{
		D3D12Renderer *realRenderer = renderer->Downcast<D3D12Renderer>();

		//Three buffers for triplebuffering
		for(size_t i = 0; i < kRND3D12UniformBufferCount; i++)
		{
			_buffers[i] = renderer->CreateBufferWithLength(size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);
		}
	}

	D3D12UniformBuffer::~D3D12UniformBuffer()
	{
		for(size_t i = 0; i < kRND3D12UniformBufferCount; i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *D3D12UniformBuffer::Advance()
	{
		_bufferIndex = (_bufferIndex + 1) % kRND3D12UniformBufferCount;
		return _buffers[_bufferIndex];

		return nullptr;
	}
}
