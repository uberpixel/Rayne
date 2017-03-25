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
	class D3D12CommandList;

	class D3D12SwapChain : public Object
	{
	public:
		friend class D3D12Window;

		D3DAPI ~D3D12SwapChain();

		D3DAPI Vector2 GetSize() const;

		D3DAPI virtual void AcquireBackBuffer();
		D3DAPI virtual void Prepare(D3D12CommandList *commandList);
		D3DAPI virtual void Finalize(D3D12CommandList *commandList);
		D3DAPI virtual void PresentBackBuffer();

		D3DAPI virtual ID3D12Resource *GetD3D12Buffer(int i) const;

		size_t GetFrameIndex() const { return _frameIndex;  }
		D3D12Framebuffer *GetFramebuffer() const { return _framebuffer; }

		uint8 GetBufferCount() const { return _bufferCount; }

	protected:
		D3DAPI D3D12SwapChain(){}

		D3D12Renderer *_renderer;
		D3D12Framebuffer *_framebuffer;
		Vector2 _size;
		uint8 _bufferCount;
		size_t _frameIndex;

	private:
		D3D12SwapChain(const Vector2 &size, HWND hwnd, D3D12Renderer *renderer, uint8 bufferCount);

		void ResizeSwapchain(const Vector2 &size, HWND hwnd);
		
		IDXGISwapChain3 *_swapChain;

		ID3D12Fence *_fence;
		UINT _fenceValues[3];
		HANDLE _fenceEvent;

		RNDeclareMetaAPI(D3D12SwapChain, D3DAPI)
	};
}

#endif /* __RAYNE_D3D12SWAPCHAIN_H__ */
