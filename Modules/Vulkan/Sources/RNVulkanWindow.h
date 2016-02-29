//
//  RNVulkanWindow.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANWINDOW_H_
#define __RAYNE_VULKANWINDOW_H_

#include "RNVulkan.h"
#include "RNVulkanBackBuffer.h"
#include "RNVulkanFramebuffer.h"

namespace RN
{
	class VulkanRenderer;
	class VulkanWindow : public Window
	{
	public:
		friend class VulkanRenderer;

		VKAPI ~VulkanWindow();

		VKAPI void SetTitle(const String *title) final;
		VKAPI Screen *GetScreen() final;

		VKAPI void Show() final;
		VKAPI void Hide() final;

		VKAPI Vector2 GetSize() const final;

		VKAPI void AcquireBackBuffer();
		VKAPI void PresentBackBuffer();

		VkSwapchainKHR GetSwapChain() const { return _swapchain; }
		VulkanFramebuffer *GetFramebuffer() const { return _framebuffer; }

	private:
		VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer);

		void InitializeSurface();
		void ResizeSwapchain(const Vector2 &size);

#if RN_PLATFORM_WINDOWS
		HWND _hwnd;
#endif
#if RN_PLATFORM_LINUX
		xcb_window_t _window;
		xcb_intern_atom_reply_t *_destroyWindow;
#endif

		VulkanRenderer *_renderer;

		std::queue<VulkanBackBuffer *> _backBuffers;
		VulkanBackBuffer *_activeBackBuffer;
		VulkanFramebuffer *_framebuffer;

		VkSurfaceKHR _surface;
		VkSurfaceFormatKHR _format;
		VkSwapchainKHR _swapchain;

		VkExtent2D _extents;

		RNDeclareMetaAPI(VulkanWindow, VKAPI)
	};
}


#endif /* __RAYNE_VULKANWINDOW_H_ */
