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

	VulkanGPUBuffer::VulkanGPUBuffer(void *data)
	{}
	VulkanGPUBuffer::~VulkanGPUBuffer()
	{}

	void *VulkanGPUBuffer::GetBuffer()
	{
		return nullptr;
	}
	void VulkanGPUBuffer::InvalidateRange(const Range &range)
	{}
	size_t VulkanGPUBuffer::GetLength() const
	{
		return 0;
	}
}
