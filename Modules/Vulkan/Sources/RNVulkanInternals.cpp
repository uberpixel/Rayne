//
//  RNVulkanInternals.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanInternals.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	RNDefineMeta(VulkanCommandBuffer, Object)

	VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool pool) : _device(device), _pool(pool)
	{

	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		vk::FreeCommandBuffers(_device, _pool, 1, &_commandBuffer);
	}

	void VulkanCommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		RNVulkanValidate(vk::BeginCommandBuffer(_commandBuffer, &cmdBufInfo));
	}

	void VulkanCommandBuffer::Reset()
	{
		RNVulkanValidate(vk::ResetCommandBuffer(_commandBuffer, 0));
	}

	void VulkanCommandBuffer::End()
	{
		RNVulkanValidate(vk::EndCommandBuffer(_commandBuffer));
	}
}
