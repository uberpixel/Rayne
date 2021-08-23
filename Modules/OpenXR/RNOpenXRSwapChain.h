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
	class OpenXRSwapChain
	{
	public:
		friend class OpenXRWindow;

		OXRAPI ~OpenXRSwapChain();

		OXRAPI virtual void ResizeSwapChain(const Vector2 &size) = 0;
		OXRAPI virtual Vector2 GetSwapChainSize() const = 0;
		OXRAPI virtual const Window::SwapChainDescriptor &GetSwapChainDescriptor() const = 0;
		OXRAPI virtual Framebuffer *GetSwapChainFramebuffer() const = 0;
		OXRAPI virtual void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic){}

	protected:
		OpenXRSwapChain(const OpenXRWindow *window);
		OpenXRSwapchainInternals *_internals;

		const OpenXRWindow *_window;

		std::function<void()> _presentEvent;
	};
}


#endif /* __RAYNE_OPENXRSWAPCHAIN_H_ */
