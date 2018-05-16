//
//  RNVulkanSwapChain.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANSWAPCHAIN_H__
#define __RAYNE_VULKANSWAPCHAIN_H__

#include "RNVulkan.h"

namespace RN
{
	class VulkanRenderer;
	class VulkanFramebuffer;
	class VulkanCommandBuffer;

	class VulkanSwapChain : public Object
	{
	public:
		friend class VulkanWindow;

		VKAPI ~VulkanSwapChain();

		VKAPI Vector2 GetSize() const;

		VKAPI virtual void AcquireBackBuffer();
		VKAPI virtual void Prepare(VkCommandBuffer commandBuffer);
		VKAPI virtual void Finalize(VkCommandBuffer commandBuffer);
		VKAPI virtual void PresentBackBuffer(VkQueue queue);

		VKAPI virtual VkImage GetVulkanColorBuffer(int i) const;
		VKAPI virtual VkImage GetVulkanDepthBuffer(int i) const;

/*		VkSemaphore GetPresentSemaphore() const { return _presentSemaphore; }
		VkSemaphore GetRenderSemaphore() const { return _renderSemaphore; }*/

		size_t GetFrameIndex() const { return _frameIndex;  }
		VulkanFramebuffer *GetFramebuffer() const { return _framebuffer; }

		uint8 GetBufferCount() const { return _descriptor.bufferCount; }
		virtual bool HasDepthBuffer() const { return false; }

	protected:
		VKAPI VulkanSwapChain(){}

		VulkanRenderer *_renderer;
		VulkanFramebuffer *_framebuffer;
		Vector2 _size;
		uint32 _frameIndex;
		Window::SwapChainDescriptor _descriptor;

		Vector2 _newSize;

	private:
		VulkanSwapChain(const Vector2 &size, HWND hwnd, VulkanRenderer *renderer, const Window::SwapChainDescriptor &descriptor);

		void CreateSurface();
		void CreateSwapChain();
		void ResizeSwapchain(const Vector2 &size);
		void SetFullscreen(bool fullscreen) const;

		HWND _hwnd;

		VkDevice _device;
		VkSemaphore _presentSemaphore;
		VkSemaphore _renderSemaphore;

		VkImage _colorBuffers[32];

		VkSurfaceKHR _surface;
		VkSurfaceFormatKHR _format;
		VkSwapchainKHR _swapchain;

		VkExtent2D _extents;

		RNDeclareMetaAPI(VulkanSwapChain, VKAPI)
	};
}

#endif /* __RAYNE_VULKANSWAPCHAIN_H__ */
