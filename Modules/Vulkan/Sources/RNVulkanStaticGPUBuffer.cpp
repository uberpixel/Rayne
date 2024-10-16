//
//  RNVulkanStaticGPUBuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanStaticGPUBuffer.h"

#include "RNVulkanInternals.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanStaticGPUBuffer, VulkanGPUBuffer)

	VulkanStaticGPUBuffer::VulkanStaticGPUBuffer(VulkanRenderer *renderer, void *data, size_t length, GPUResource::UsageOptions usageOption, GPUResource::AccessOptions accessOptions)
	 : _length(length), _renderer(renderer), _mappedBuffer(nullptr), _isHostVisible(false), _stagingBuffer(VK_NULL_HANDLE), _stagingAllocation(VK_NULL_HANDLE)
	{
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = length;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		//If write only, prefer host coherent memory if available
		if(accessOptions == GPUResource::AccessOptions::WriteOnly) allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;

		switch (usageOption)
		{
			case UsageOptions::Uniform:
				bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
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

	VulkanStaticGPUBuffer::~VulkanStaticGPUBuffer()
	{
		UnmapBuffer();

		//Delay the buffer deletion to end of frame to ensure that it is not in use anymore.
		VmaAllocation allocation = _allocation;
		VkBuffer buffer = _buffer;
		VulkanRenderer *renderer = _renderer;
		renderer->AddFrameFinishedCallback([renderer, buffer, allocation]() {
			vmaDestroyBuffer(renderer->_internals->memoryAllocator, buffer, allocation);
		});
	}

	void *VulkanStaticGPUBuffer::GetBuffer()
	{
		Lock();
		if(!_mappedBuffer)
		{
			if(_isHostVisible)
			{
				void *tempBuffer = nullptr;
				vmaMapMemory(_renderer->_internals->memoryAllocator, _allocation, &tempBuffer);
				_mappedBuffer = tempBuffer;
			}
			else
			{
				VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				stagingBufferInfo.size = _length;
				stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VmaAllocationCreateInfo stagingAllocInfo = {};
				stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
				stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

				VmaAllocationInfo allocationInfo;
				RNVulkanValidate(vmaCreateBuffer(_renderer->_internals->memoryAllocator, &stagingBufferInfo, &stagingAllocInfo, &_stagingBuffer, &_stagingAllocation, &allocationInfo));

				_mappedBuffer = allocationInfo.pMappedData;
			}
		}

		void *mappedBuffer = _mappedBuffer;
		Unlock();

		return mappedBuffer;
	}

	VkBuffer VulkanStaticGPUBuffer::GetVulkanBuffer() const
	{
		return _buffer;
	}

	void VulkanStaticGPUBuffer::UnmapBuffer()
	{
		Lock();
		if(!_mappedBuffer)
		{
			Unlock();
			return;
		}

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

		Unlock();
	}

	void VulkanStaticGPUBuffer::InvalidateRange(const Range &range)
	{
		Lock();
		if(!_mappedBuffer || _isHostVisible)
		{
			Unlock();
			return;
		}

		Unlock();

/*		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		VkMappedMemoryRange memoryRange;
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.pNext = nullptr;
		memoryRange.memory = _memory;
		memoryRange.offset = range.origin;
		memoryRange.size = range.length;

		RNVulkanValidate(vk::InvalidateMappedMemoryRanges(device, 1, &memoryRange));*/
	}

	void VulkanStaticGPUBuffer::FlushRange(const Range &range)
	{
		Lock();
		if(!_mappedBuffer || _isHostVisible)
		{
			Unlock();
			return;
		}

		VulkanCommandBuffer *commandBuffer = _renderer->StartResourcesCommandBuffer();

		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _length;
		vk::CmdCopyBuffer(commandBuffer->GetCommandBuffer(), _stagingBuffer, _buffer, 1, &copyRegion);

		Unlock();
		_renderer->EndResourcesCommandBuffer();

/*		VkDevice device = _renderer->GetVulkanDevice()->GetDevice();
		VkMappedMemoryRange memoryRange;
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.pNext = nullptr;
		memoryRange.memory = _memory;
		memoryRange.offset = range.origin;
		memoryRange.size = range.length;

		RNVulkanValidate(vk::FlushMappedMemoryRanges(device, 1, &memoryRange));*/
	}

	size_t VulkanStaticGPUBuffer::GetLength() const
	{
		return _length;
	}
}
