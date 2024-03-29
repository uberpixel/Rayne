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
//#include "RNVulkanBackBuffer.h"

namespace RN
{
	class VulkanRenderer;
	class VulkanFramebuffer;
	class VulkanSwapChain;

	class VulkanWindow : public Window
	{
	public:
		friend class VulkanRenderer;

		VKAPI ~VulkanWindow();

		VKAPI void SetTitle(const String *title) final;
		VKAPI Screen *GetScreen() final;

		VKAPI void Show() final;
		VKAPI void Hide() final;
		VKAPI void SetFullscreen(bool fullscreen) final;

		VKAPI Vector2 GetSize() const final;
		VKAPI Framebuffer *GetFramebuffer() const final;

		VKAPI void UpdateSize();

		VKAPI virtual const Window::SwapChainDescriptor &GetSwapChainDescriptor() const override;

		VKAPI uint64 GetWindowHandle() const final;

#if RN_PLATFORM_WINDOWS
		HWND GetHWND() const { return _hwnd; }
#endif
		VulkanSwapChain *GetSwapChain() const { return _swapChain; }

		void SetPressedAlt(bool pressed) { _keyPressedAlt = pressed; }
		void SetPressedReturn(bool pressed) { _keyPressedReturn = pressed; }

		bool GetPressedAlt() const { return _keyPressedAlt; }
		bool GetPressedReturn() const { return _keyPressedReturn; }

		//TODO: Should fetch the current fullscreen state from the swapchain
		VKAPI bool GetFullscreen() const;

	private:
		VulkanWindow(const Vector2 &size, Screen *screen, VulkanRenderer *renderer, const Window::SwapChainDescriptor &descriptor);

		VulkanSwapChain *_swapChain;
		VulkanRenderer *_renderer;

		bool _keyPressedAlt;
		bool _keyPressedReturn;

#if RN_PLATFORM_WINDOWS
		HWND _hwnd;
#endif
#if RN_PLATFORM_LINUX
		xcb_window_t _window;
		xcb_intern_atom_reply_t *_destroyWindow;
#endif
#if RN_PLATFORM_ANDROID
		ANativeWindow *_window;
#endif

		RNDeclareMetaAPI(VulkanWindow, VKAPI)
	};
}


#endif /* __RAYNE_VULKANWINDOW_H_ */
