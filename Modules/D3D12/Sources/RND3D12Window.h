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

	class D3D12Window : public Window
	{
	public:
		friend class D3D12Renderer;
		friend class D3D12RendererInternals;

		D3DAPI void SetTitle(const String *title) final;
		D3DAPI Screen *GetScreen() final;

		D3DAPI void Show() final;
		D3DAPI void Hide() final;

		D3DAPI Vector2 GetSize() const final;

	private:
		D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer);

		HWND _hwnd;
		D3D12Renderer *_renderer;

		RNDeclareMeta(D3D12Window)
	};
}

#endif /* __RAYNE_D3D12WINDOW_H__ */
