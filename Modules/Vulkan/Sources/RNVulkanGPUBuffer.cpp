//
//  RNVulkanGPUBuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanGPUBuffer.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanGPUBuffer, GPUBuffer)

	VulkanGPUBuffer::VulkanGPUBuffer(VulkanRenderer *renderer, void *data, size_t length, GPUResource::UsageOptions usageOption)
	 : _length(length), _renderer(renderer), _mappedBuffer(nullptr)
	{
		VkDevice device = renderer->GetVulkanDevice()->GetDevice();
		VkMemoryRequirements memoryRequirements;

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = NULL;
		memoryAllocateInfo.allocationSize = 0;
		memoryAllocateInfo.memoryTypeIndex = 0;

		VkBufferUsageFlags usage;
		switch(usageOption)
		{
			case UsageOptions::Uniform:
				usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				break;
			case UsageOptions::Vertex:
				usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				break;
			case UsageOptions::Index:
				usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				break;
		}

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = NULL;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.size = length;
		bufferCreateInfo.flags = 0;

		RNVulkanValidate(vk::CreateBuffer(device, &bufferCreateInfo, nullptr, &_buffer));
		vk::GetBufferMemoryRequirements(device, _buffer, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;

		renderer->GetVulkanDevice()->GetMemoryWithType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryAllocateInfo.memoryTypeIndex);
		RNVulkanValidate(vk::AllocateMemory(device, &memoryAllocateInfo, nullptr, &_memory));
		if(data != nullptr)
		{
			void *mapped;
			RNVulkanValidate(vk::MapMemory(device, _memory, 0, length, 0, &mapped));
			memcpy(mapped, data, length);
			vk::UnmapMemory(device, _memory);
		}
		RNVulkanValidate(vk::BindBufferMemory(device, _buffer, _memory, 0));
	}

	VulkanGPUBuffer::~VulkanGPUBuffer()
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		vk::DestroyBuffer(device, _buffer, nullptr);
		vk::FreeMemory(device, _memory, nullptr);
	}

	void *VulkanGPUBuffer::GetBuffer()
	{
		if(!_mappedBuffer)
		{
			VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
			RNVulkanValidate(vk::MapMemory(device, _memory, 0, _length, 0, &_mappedBuffer));
		}
		return _mappedBuffer;
	}

	void VulkanGPUBuffer::InvalidateRange(const Range &range)
	{
		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		vk::UnmapMemory(device, _memory);
		_mappedBuffer = nullptr;
	}

	size_t VulkanGPUBuffer::GetLength() const
	{
		return _length;
	}
}
