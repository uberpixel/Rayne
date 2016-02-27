//
//  RNVulkanBackBuffer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANBACKBUFFER_H_
#define __RAYNE_VULKANBACKBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanDispatchTable.h"

namespace RN
{
	class VulkanBackBuffer
	{
	public:
		VKAPI VulkanBackBuffer(VkDevice device, VkSwapchainKHR swapchain);
		VKAPI ~VulkanBackBuffer();

		VKAPI void WaitForPresentFence();
		VKAPI void AcquireNextImage();
		VKAPI void Present(VkQueue queue);

		VkFence GetPresentFence() const { return _presentFence; }

	private:
		VkSwapchainKHR _swapchain;

		VkDevice _device;
		VkSemaphore _acquireSemaphore;
		VkSemaphore _renderSemaphore;
		VkFence _presentFence;

		uint32_t _imageIndex;
	};
}


#endif /* __RAYNE_VULKANBACKBUFFER_H_ */
