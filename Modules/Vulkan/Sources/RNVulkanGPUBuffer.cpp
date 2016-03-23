//
//  RNVulkanGPUBuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanGPUBuffer.h"

namespace RN
{
	RNDefineMeta(VulkanGPUBuffer, GPUBuffer)

	VulkanGPUBuffer::VulkanGPUBuffer(void *data, size_t length)
	{
		_data = data;
		_length = length;
	}
	VulkanGPUBuffer::~VulkanGPUBuffer()
	{
		free(_data);
	}

	void *VulkanGPUBuffer::GetBuffer()
	{
		return _data;
	}
	void VulkanGPUBuffer::InvalidateRange(const Range &range)
	{}
	size_t VulkanGPUBuffer::GetLength() const
	{
		return _length;
	}
}
