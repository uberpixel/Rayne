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
	VulkanBackBuffer::VulkanBackBuffer(VkDevice device, VkSwapchainKHR swapchain) :
		_device(device),
		_swapchain(swapchain)
	{
		VkSemaphoreCreateInfo semaphore = {};
		semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		RNVulkanValidate(vk::CreateSemaphore(device, &semaphore, nullptr, &_presentSemaphore));
		RNVulkanValidate(vk::CreateSemaphore(device, &semaphore, nullptr, &_renderSemaphore));
	}

	VulkanBackBuffer::~VulkanBackBuffer()
	{
		vk::DestroySemaphore(_device, _presentSemaphore, nullptr);
		vk::DestroySemaphore(_device, _renderSemaphore, nullptr);
	}

	void VulkanBackBuffer::AcquireNextImage()
	{
		RNVulkanValidate(vk::AcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _presentSemaphore, VK_NULL_HANDLE, &_imageIndex));
	}

	void VulkanBackBuffer::Present(VkQueue queue)
	{
		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &_renderSemaphore;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &_swapchain;
		present_info.pImageIndices = &_imageIndex;

		RNVulkanValidate(vk::QueuePresentKHR(queue, &present_info));
		RNVulkanValidate(vk::QueueWaitIdle(queue));
	}
}
