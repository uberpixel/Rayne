//
//  RNVulkanConstantBuffer.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanConstantBuffer.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanGPUBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanConstantBuffer, Object)

	VulkanConstantBuffer::VulkanConstantBuffer(size_t size) :
		_bufferIndex(0),
		_size(size)
	{
		VulkanRenderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
		GPUBuffer *buffer = realRenderer->CreateBufferWithLength(_size, GPUResource::UsageOptions::Uniform, GPUResource::AccessOptions::ReadWrite);
		_buffers.push_back(buffer);
		_bufferFrames.push_back(0);
	}

	VulkanConstantBuffer::~VulkanConstantBuffer()
	{
		for(size_t i = 0; i < _buffers.size(); i ++)
			_buffers[i]->Release();
	}

	GPUBuffer *VulkanConstantBuffer::Advance(size_t currentFrame, size_t completedFrame)
	{
		_bufferIndex = (_bufferIndex + 1) % _buffers.size();

		if(_bufferFrames[_bufferIndex] > completedFrame)
		{
			VulkanRenderer *realRenderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
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
	}
}
