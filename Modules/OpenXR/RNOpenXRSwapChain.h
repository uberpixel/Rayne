//
//  RNOpenXRSwapChain.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_OPENXRSWAPCHAIN_H_
#define __RAYNE_OPENXRSWAPCHAIN_H_

#include "RNOpenXR.h"

namespace RN
{
	struct OpenXRSwapchainInternals;
	class OpenXRWindow;
	class OpenXRSwapChain
	{
	public:
		friend OpenXRWindow;
		friend class OpenXRCompositorLayer;

		enum SwapChainType
		{
			Metal,
			D3D12,
			Vulkan
		};

		OXRAPI ~OpenXRSwapChain();

		OXRAPI virtual void ResizeSwapChain(const Vector2 &size) = 0;
		OXRAPI virtual Vector2 GetSwapChainSize() const = 0;
		OXRAPI virtual const Window::SwapChainDescriptor &GetSwapChainDescriptor() const = 0;
		OXRAPI virtual Framebuffer *GetSwapChainFramebuffer() const = 0;
		OXRAPI virtual void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic){}

		OXRAPI virtual void SetActive(bool active) { _isActive = active; }

	protected:
		OpenXRSwapChain(const OpenXRWindow *window, SwapChainType type);
		OpenXRSwapchainInternals *_internals;

		const OpenXRWindow *_xrWindow;
		bool _isActive;
		bool _hasContent;

		SwapChainType _swapChainType;

		std::function<void()> _presentEvent;
	};
}


#endif /* __RAYNE_OPENXRSWAPCHAIN_H_ */
