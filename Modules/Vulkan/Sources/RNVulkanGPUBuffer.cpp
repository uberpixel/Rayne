//
//  RNVulkanGPUBuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanGPUBuffer.h"

#include "RNVulkanInternals.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanGPUBuffer, GPUBuffer)

	VulkanGPUBuffer::VulkanGPUBuffer(VulkanRenderer *renderer, void *data, size_t length, GPUResource::UsageOptions usageOption)
	 : _length(length), _renderer(renderer), _mappedBuffer(nullptr), _isHostVisible(false), _stagingBuffer(VK_NULL_HANDLE), _stagingAllocation(VK_NULL_HANDLE)
	{
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = length;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		switch (usageOption)
		{
			case UsageOptions::Uniform:
				bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
				allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
				allocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
				break;
			case UsageOptions::Vertex:
				bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				break;
			case UsageOptions::Index:
				bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				break;
		}

		RNVulkanValidate(vmaCreateBuffer(renderer->_internals->memoryAllocator, &bufferInfo, &allocCreateInfo, &_buffer, &_allocation, nullptr));

		VkMemoryPropertyFlags memoryPropertyFlags;
		vmaGetAllocationMemoryProperties(renderer->_internals->memoryAllocator, _allocation, &memoryPropertyFlags);
		if(memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			_isHostVisible = true;
		}

		if(data != nullptr)
		{
			memcpy(GetBuffer(), data, length);
			FlushRange(Range(0, length));
			UnmapBuffer();
		}
	}

	VulkanGPUBuffer::~VulkanGPUBuffer()
	{
		UnmapBuffer();
		vmaDestroyBuffer(_renderer->_internals->memoryAllocator, _buffer, _allocation);
	}

	void *VulkanGPUBuffer::GetBuffer()
	{
		if(!_mappedBuffer)
		{
			if(_isHostVisible)
			{
				vmaMapMemory(_renderer->_internals->memoryAllocator, _allocation, &_mappedBuffer);
			}
			else
			{
				VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				stagingBufferInfo.size = _length;
				stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VmaAllocationCreateInfo stagingAllocInfo = {};
				stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
				stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

				VkBuffer stagingBuffer;
				VmaAllocation stagingAllocation;
				VmaAllocationInfo allocationInfo;
				RNVulkanValidate(vmaCreateBuffer(_renderer->_internals->memoryAllocator, &stagingBufferInfo, &stagingAllocInfo, &_stagingBuffer, &_stagingAllocation, &allocationInfo));

				_mappedBuffer = allocationInfo.pMappedData;
			}
		}
		return _mappedBuffer;
	}

	VkBuffer VulkanGPUBuffer::GetVulkanBuffer() const
	{
		return _buffer;
	}

	void VulkanGPUBuffer::UnmapBuffer()
	{
		if(!_mappedBuffer) return;

		if(_isHostVisible)
		{
			vmaUnmapMemory(_renderer->_internals->memoryAllocator, _allocation);
		}
		else
		{
			VmaAllocation stagingAllocation = _stagingAllocation;
			VkBuffer stagingBuffer = _stagingBuffer;
			VulkanRenderer *renderer = _renderer;
			renderer->AddFrameFinishedCallback([renderer, stagingBuffer, stagingAllocation]() {
				vmaDestroyBuffer(renderer->_internals->memoryAllocator, stagingBuffer, stagingAllocation);
			});

			_stagingBuffer = VK_NULL_HANDLE;
			_stagingAllocation = VK_NULL_HANDLE;
		}
		_mappedBuffer = nullptr;
	}

	void VulkanGPUBuffer::InvalidateRange(const Range &range)
	{
		if(!_mappedBuffer || _isHostVisible) return;

/*		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		VkMappedMemoryRange memoryRange;
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.pNext = nullptr;
		memoryRange.memory = _memory;
		memoryRange.offset = range.origin;
		memoryRange.size = range.length;

		RNVulkanValidate(vk::InvalidateMappedMemoryRanges(device, 1, &memoryRange));*/
	}

	void VulkanGPUBuffer::FlushRange(const Range &range)
	{
		if(!_mappedBuffer || _isHostVisible) return;

		VulkanCommandBuffer *commandBuffer = _renderer->GetCommandBuffer();
		commandBuffer->Begin();

		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _length;
		vk::CmdCopyBuffer(commandBuffer->GetCommandBuffer(), _stagingBuffer, _buffer, 1, &copyRegion);

		commandBuffer->End();

		_renderer->SubmitCommandBuffer(commandBuffer);

/*		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		VkMappedMemoryRange memoryRange;
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.pNext = nullptr;
		memoryRange.memory = _memory;
		memoryRange.offset = range.origin;
		memoryRange.size = range.length;

		RNVulkanValidate(vk::FlushMappedMemoryRanges(device, 1, &memoryRange));*/
	}

	size_t VulkanGPUBuffer::GetLength() const
	{
		return _length;
	}
}
