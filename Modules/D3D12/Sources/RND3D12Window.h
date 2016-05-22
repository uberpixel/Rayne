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

		D3DAPI void AcquireBackBuffer();
		D3DAPI void PresentBackBuffer();

		ID3D12CommandQueue *GetCommandQueue() const { return _commandQueue; }
		ID3D12CommandAllocator *GetCommandAllocator() const { return _commandAllocators[_frameIndex]; }
		ID3D12GraphicsCommandList *GetCommandList() const { return _commandList; }

		HWND GetHWND() const { return _hwnd;  }
		size_t GetFrameIndex() const { return _frameIndex;  }
		D3D12Framebuffer *GetFramebuffer() const { return _framebuffer;  }

	private:
		D3D12Window(const Vector2 &size, Screen *screen, D3D12Renderer *renderer);

		void ResizeSwapchain(const Vector2 &size);

		HWND _hwnd;
		D3D12Renderer *_renderer;
		D3D12Framebuffer *_framebuffer;

		ID3D12CommandAllocator *_commandAllocators[3];
		ID3D12GraphicsCommandList *_commandList;

		ID3D12CommandQueue *_commandQueue;
		IDXGISwapChain3 *_swapChain;

		ID3D12Fence *_fence;
		UINT _fenceValues[3];
		HANDLE _fenceEvent;

		size_t _frameIndex;

		RNDeclareMetaAPI(D3D12Window, D3DAPI)
	};
}

#endif /* __RAYNE_D3D12WINDOW_H__ */
