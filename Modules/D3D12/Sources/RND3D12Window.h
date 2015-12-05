//
//  RND3D12Window.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12WINDOW_H__
#define __RAYNE_D3D12WINDOW_H__

#include <Rayne.h>

namespace RN
{
	class D3D12Renderer;
	struct D3D12WindowInternals;

	class D3D12Window : public Window
	{
	public:
		friend class D3D12Renderer;

		void SetTitle(const String *title) final;
		Screen *GetScreen() final;

		void Show() final;
		void Hide() final;

		Vector2 GetSize() const final;

	private:
		D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer);

		PIMPL<D3D12WindowInternals> _internals;
		D3D12Renderer *_renderer;

		RNDeclareMeta(D3D12Window)
	};
}

#endif /* __RAYNE_D3D12WINDOW_H__ */
