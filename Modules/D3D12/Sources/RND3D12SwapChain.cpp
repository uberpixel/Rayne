//
//  RND3D12SwapChain.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12SwapChain.h"
#include "RND3D12Renderer.h"
#include "RND3D12Framebuffer.h"

namespace RN
{
	RNDefineMeta(D3D12SwapChain, Object)

		D3D12SwapChain::D3D12SwapChain(const Vector2 &size, HWND hwnd, D3D12Renderer *renderer, uint8 bufferCount) :
		_renderer(renderer),
		_frameIndex(0),
		_size(size),
		_bufferCount(bufferCount)
	{
		for(int i = 0; i < bufferCount; i++)
			_fenceValues[i] = 0;

		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();
		device->CreateFence(_fenceValues[_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		_fenceValues[_frameIndex] ++;
		_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

		ResizeSwapchain(size, hwnd);
	}

	D3D12SwapChain::~D3D12SwapChain()
	{
		
	}

	void D3D12SwapChain::ResizeSwapchain(const Vector2 &size, HWND hwnd)
	{
		_size = size;

		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();
		IDXGIFactory4 *factory = _renderer->GetD3D12Descriptor()->GetFactory();

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = _bufferCount;
		swapChainDesc.BufferDesc.Width = size.x;
		swapChainDesc.BufferDesc.Height = size.y;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = true;

		IDXGISwapChain *swapChain;
		factory->CreateSwapChain(_renderer->GetCommandQueue(), &swapChainDesc, &swapChain);
		_swapChain = static_cast<IDXGISwapChain3 *>(swapChain);
		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		Framebuffer::Descriptor descriptor;
		descriptor.options = Framebuffer::Options::PrivateStorage;
		descriptor.colorFormat = Texture::Format::RGBA8888;

		_framebuffer = new D3D12Framebuffer(size, descriptor, this, _renderer);
	}

	Vector2 D3D12SwapChain::GetSize() const
	{
		return _size;
	}

	void D3D12SwapChain::AcquireBackBuffer()
	{
		const UINT64 fenceValue = _fenceValues[_frameIndex];
		_renderer->GetCommandQueue()->Signal(_fence, fenceValue);

		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		UINT completedFenceValue = _fence->GetCompletedValue();
		if(completedFenceValue < _fenceValues[_frameIndex])
		{
			_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
			WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
		}

		_fenceValues[_frameIndex] = fenceValue + 1;
	}

	void D3D12SwapChain::PresentBackBuffer()
	{
		_swapChain->Present(0, 0); //Use 1, 0 for vsync
	}
}
