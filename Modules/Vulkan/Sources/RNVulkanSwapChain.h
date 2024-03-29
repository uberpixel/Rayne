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
		VKAPI virtual VkImage GetVulkanFragmentDensityBuffer(int i, uint32 &width, uint32 &height) const;

		VkSemaphore GetCurrentPresentSemaphore() const { return _presentSemaphores[_semaphoreIndex]; }
		VkSemaphore GetCurrentRenderSemaphore() const { return _renderSemaphores[_semaphoreIndex]; }

		size_t GetFrameIndex() const { return _frameIndex;  }
		VulkanFramebuffer *GetFramebuffer() const { return _framebuffer; }

		uint8 GetBufferCount() const { return _descriptor.bufferCount; }
		virtual bool HasDepthBuffer() const { return false; }

		virtual const Window::SwapChainDescriptor &GetSwapChainDescriptor() const
		{
			return _descriptor;
		}

	protected:
		VKAPI VulkanSwapChain();
		VKAPI void CreateSemaphores();

		VulkanRenderer *_renderer;
		VkDevice _device;
		VulkanFramebuffer *_framebuffer;
		Vector2 _size;
		uint32 _frameIndex;
		uint32 _semaphoreIndex;
		Window::SwapChainDescriptor _descriptor;

		std::vector<VkSemaphore> _presentSemaphores;
        std::vector<VkSemaphore> _renderSemaphores;

		Vector2 _newSize;

	private:
#if RN_PLATFORM_WINDOWS
		VulkanSwapChain(const Vector2& size, HWND hwnd, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor);
		HWND _hwnd;
#elif RN_PLATFORM_ANDROID
		VulkanSwapChain(const Vector2& size, ANativeWindow *window, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor);
		ANativeWindow *_window;
#elif RN_PLATFORM_LINUX
		VulkanSwapChain(const Vector2& size, xcb_window_t window, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor);
		xcb_window_t _window;
#else
		VulkanSwapChain(const Vector2& size, VulkanRenderer* renderer, const Window::SwapChainDescriptor& descriptor);
#endif

		void CreateSurface();
		void CreateSwapChain();
		void ResizeSwapchain(const Vector2 &size);
		void SetFullscreen(bool fullscreen);
		bool GetIsFullscreen() const { return _isFullscreen; }

		VkImage _colorBuffers[32];

		VkSurfaceKHR _surface;
		VkSurfaceFormatKHR _format;
		VkSwapchainKHR _swapchain;

		VkExtent2D _extents;
		bool _isFullscreen;
		Vector2 _preFullscreenWindowSize;
		Vector2 _preFullscreenWindowPosition;
		uint64 _preFullscreenWindowStyle;

		RNDeclareMetaAPI(VulkanSwapChain, VKAPI)
	};
}

#endif /* __RAYNE_VULKANSWAPCHAIN_H__ */
