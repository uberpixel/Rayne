//
//  RND3D12SwapChain.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12SWAPCHAIN_H__
#define __RAYNE_D3D12SWAPCHAIN_H__

#include "RND3D12.h"

namespace RN
{
	class D3D12Renderer;
	class D3D12Framebuffer;

	class D3D12SwapChain : public Object
	{
	public:
		friend class D3D12Window;

		D3DAPI ~D3D12SwapChain();

		D3DAPI Vector2 GetSize() const;

		D3DAPI void AcquireBackBuffer();
		D3DAPI void PresentBackBuffer();

		size_t GetFrameIndex() const { return _frameIndex;  }
		D3D12Framebuffer *GetFramebuffer() const { return _framebuffer;  }
		const UINT GetCurrentFenceValue() const { return _fenceValues[_frameIndex]; }
		const UINT GetCompletedFenceValue() const { return _completedFenceValue; }

		IDXGISwapChain3 *GetD3D12SwapChain() const { return _swapChain; }
		uint8 GetBufferCount() const { return _bufferCount; }

	private:
		D3D12SwapChain(const Vector2 &size, HWND hwnd, D3D12Renderer *renderer, uint8 bufferCount);

		void ResizeSwapchain(const Vector2 &size, HWND hwnd);

		Vector2 _size;
		uint8 _bufferCount;

		D3D12Renderer *_renderer;
		D3D12Framebuffer *_framebuffer;
		IDXGISwapChain3 *_swapChain;

		ID3D12Fence *_fence;
		UINT _fenceValues[3];
		UINT _completedFenceValue;
		HANDLE _fenceEvent;

		size_t _frameIndex;

		RNDeclareMetaAPI(D3D12SwapChain, D3DAPI)
	};
}

#endif /* __RAYNE_D3D12SWAPCHAIN_H__ */
