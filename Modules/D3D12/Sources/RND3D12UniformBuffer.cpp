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

	D3D12UniformBuffer::D3D12UniformBuffer(size_t size) :
		_bufferIndex(0),
		_size(size)
	{
		D3D12Renderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		GPUBuffer *buffer = realRenderer->CreateBufferWithLength(_size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);
		_buffers.push_back(buffer);
		_bufferFrames.push_back(0);
	}

	D3D12UniformBuffer::~D3D12UniformBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *D3D12UniformBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		_bufferIndex = (_bufferIndex + 1) % _buffers.size();

		if(_bufferFrames[_bufferIndex] > completedFrame)
		{
			D3D12Renderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
			GPUBuffer *buffer = realRenderer->CreateBufferWithLength(_size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);

			_bufferIndex += 1;
			if(_bufferIndex >= _buffers.size())
			{
				_buffers.push_back(buffer);
				_bufferFrames.push_back(currentFrame);
			}
			else
			{
				_buffers.insert(_buffers.begin() + _bufferIndex, buffer);
				_bufferFrames.insert(_bufferFrames.begin() + _bufferIndex, currentFrame);
			}
		}
		else
		{
			_bufferFrames[_bufferIndex] = currentFrame;
		}

		return _buffers[_bufferIndex];

		return nullptr;
	}
}
