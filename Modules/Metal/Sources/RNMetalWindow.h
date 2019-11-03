//
//  RNMetalWindow.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALWINDOW_H__
#define __RAYNE_METALWINDOW_H__

#include "RNMetal.h"

namespace RN
{
	class MetalRenderer;
	class MetalSwapChain;
	struct MetalWindowInternals;

	class MetalWindow : public Window
	{
	public:
		friend class MetalRenderer;

		MTLAPI void SetTitle(const String *title) final;
		MTLAPI Screen *GetScreen() final;

		MTLAPI void Show() final;
		MTLAPI void Hide() final;

		MTLAPI void SetFullscreen(bool fullscreen) final;

		MTLAPI Vector2 GetSize() const final;
		MTLAPI Framebuffer *GetFramebuffer() const final;

		MetalSwapChain *GetSwapChain() const { return _swapChain; }
		virtual const Window::SwapChainDescriptor &GetSwapChainDescriptor() const override;
		
		MTLAPI uint64 GetWindowHandle() const final;

	private:
		MetalWindow(const Vector2 &size, Screen *screen, MetalRenderer *renderer, const Window::SwapChainDescriptor &descriptor);

		PIMPL<MetalWindowInternals> _internals;
		MetalRenderer *_renderer;

		MetalSwapChain *_swapChain;

		RNDeclareMetaAPI(MetalWindow, MTLAPI)
	};
}

#endif /* __RAYNE_METALWINDOW_H__ */
