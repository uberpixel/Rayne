//
//  RNVulkanBackBuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanBackBuffer.h"

namespace RN
{
	VulkanBackBuffer::VulkanBackBuffer(VkDevice device) :
		_device(device)
	{
		VkSemaphoreCreateInfo semaphore = {};
		semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence = {};
		fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		RNVulkanValidate(vk::CreateSemaphore(device, &semaphore, nullptr, &_acquireSemaphore));
		RNVulkanValidate(vk::CreateSemaphore(device, &semaphore, nullptr, &_renderSemaphore));
		RNVulkanValidate(vk::CreateFence(device, &fence, nullptr, &_presentFence));
	}

	VulkanBackBuffer::~VulkanBackBuffer()
	{
		vk::DestroySemaphore(_device, _acquireSemaphore, nullptr);
		vk::DestroySemaphore(_device, _renderSemaphore, nullptr);
		vk::DestroyFence(_device, _presentFence, nullptr);
	}

	void VulkanBackBuffer::WaitForPresentFence()
	{
		RNVulkanValidate(vk::WaitForFences(_device, 1, &_presentFence, VK_TRUE, UINT64_MAX));
		RNVulkanValidate(vk::ResetFences(_device, 1, &_presentFence));
	}

	void VulkanBackBuffer::AcquireNextImage(VkSwapchainKHR swapchain)
	{
		RNVulkanValidate(vk::AcquireNextImageKHR(_device, swapchain, UINT64_MAX, _acquireSemaphore, VK_NULL_HANDLE, &_imageIndex));
	}
}
