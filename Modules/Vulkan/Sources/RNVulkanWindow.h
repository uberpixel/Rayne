//
//  RNVulkanWindow.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANWINDOW_H_
#define __RAYNE_VULKANWINDOW_H_

#include <queue>
#include "RNVulkan.h"
#include "RNVulkanBackBuffer.h"

#define kRNVulkanBackBufferCount 3

namespace RN
{
	class VulkanRenderer;
	class VulkanWindow : public Window
	{
	public:
		friend class VulkanRenderer;

		~VulkanWindow();

		void SetTitle(const String *title) final;
		Screen *GetScreen() final;

		void Show() final;
		void Hide() final;

		Vector2 GetSize() const final;

		void AcquireBackBuffer();
		void PresentBackBuffer();

	private:
		VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer);

		void InitializeSurface();
		void ResizeSwapchain(const Vector2 &size);

		HWND _hwnd;
		VulkanRenderer *_renderer;

		std::queue<VulkanBackBuffer *> _backBuffers;
		VulkanBackBuffer *_activeBackBuffer;

		VkSurfaceKHR _surface;
		VkSurfaceFormatKHR _format;
		VkSwapchainKHR _swapchain;

		VkExtent2D _extents;

		RNDeclareMetaAPI(VulkanWindow, VKAPI)
	};
}


#endif /* __RAYNE_VULKANWINDOW_H_ */
