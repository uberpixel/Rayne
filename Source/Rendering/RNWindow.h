//
//  RNWindow.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_WINDOW_H_
#define __RAYNE_WINDOW_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Rendering/RNFramebuffer.h"
#include "../System/RNScreen.h"

#define kRNWindowDidChangeSize RNCSTR("kRNWindowDidChangeSize")
#define kRNWindowWillDestroy RNCSTR("kRNWindowWillDestroy")

namespace RN
{
	class Window : public Object
	{
	public:
		//TODO: Maybe make a RNSwapChain and move this there?
		struct SwapChainDescriptor
		{
			SwapChainDescriptor(Texture::Format colorFormat = Texture::Format::BGRA_8_SRGB, Texture::Format depthStencilFormat = Texture::Format::Invalid) :
				colorFormat(colorFormat), depthStencilFormat(depthStencilFormat), bufferCount(4), layerCount(1), usageHint(Texture::UsageHint::RenderTarget), vsync(true), wantsFullscreen(false) {}
			Texture::Format colorFormat;
			Texture::Format depthStencilFormat;
			uint8 bufferCount;
			uint8 layerCount;
			Texture::UsageHint usageHint;
			bool vsync;
			bool wantsFullscreen;
		};

		RNAPI ~Window();

		RNAPI virtual void SetTitle(const String *title) = 0;
		RNAPI virtual Screen *GetScreen() = 0;

		RNAPI virtual void Show() = 0;
		RNAPI virtual void Hide() = 0;

		RNAPI virtual void SetFullscreen(bool fullscreen) = 0;

		RNAPI virtual Vector2 GetSize() const = 0;

		RNAPI virtual Framebuffer *GetFramebuffer() const = 0;
		RNAPI virtual bool SetSwapChainReleaseCallback(std::function<void()> callback);

		RNAPI virtual const Window::SwapChainDescriptor &GetSwapChainDescriptor() const = 0;

		RNAPI virtual uint64 GetWindowHandle() const = 0;

		// Normalized content coordinates, with (0, 0) at the top left. Currently macOS only.
		// Returns false if unsupported, off the main thread, unfocused, or outside the content view.
		RNAPI bool GetMousePosition(Vector2 &position) const;

		RNAPI void TrapMouseCursor();
		RNAPI void ReleaseMouseCursor();

		RNAPI void ShowMouseCursor();
		RNAPI void HideMouseCursor();

	protected:
		RNAPI Window();
		RNAPI Window(Screen *screen);

	private:
		Screen *_screen;

		__RNDeclareMetaInternal(Window)
	};
} // namespace RN


#endif /* __RAYNE_WINDOW_H_ */
