//
//  RND3D12Window.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12WINDOW_H__
#define __RAYNE_D3D12WINDOW_H__

#include "RND3D12.h"

namespace RN
{
	class D3D12Renderer;
	class D3D12Framebuffer;
	class D3D12SwapChain;

	class D3D12Window : public Window
	{
	public:
		friend class D3D12Renderer;

		D3DAPI ~D3D12Window();

		D3DAPI void SetTitle(const String *title) final;
		D3DAPI Screen *GetScreen() final;

		D3DAPI void Show() final;
		D3DAPI void Hide() final;

		D3DAPI Vector2 GetSize() const final;
		D3DAPI Framebuffer *GetFramebuffer() const final;

		D3DAPI void UpdateSize() const;

		HWND GetHWND() const { return _hwnd; }
		D3D12SwapChain *GetSwapChain() const { return _swapChain; }

	private:
		D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer, const Window::SwapChainDescriptor &descriptor);
		HWND _hwnd;

		D3D12SwapChain *_swapChain;

		RNDeclareMetaAPI(D3D12Window, D3DAPI)
	};
}

#endif /* __RAYNE_D3D12WINDOW_H__ */
