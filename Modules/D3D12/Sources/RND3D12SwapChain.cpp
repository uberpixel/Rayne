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
#include "RND3D12Internals.h"

namespace RN
{
	RNDefineMeta(D3D12SwapChain, Object)

		D3D12SwapChain::D3D12SwapChain(const Vector2 &size, HWND hwnd, D3D12Renderer *renderer, const Window::SwapChainDescriptor &descriptor) :
		_renderer(renderer),
		_frameIndex(0),
		_size(size),
		_descriptor(descriptor)
	{
		for(int i = 0; i < descriptor.bufferCount; i++)
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
		swapChainDesc.BufferCount = _descriptor.bufferCount;
		swapChainDesc.BufferDesc.Width = size.x;
		swapChainDesc.BufferDesc.Height = size.y;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;	//no vsync
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1; //no vsync
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = true;

		IDXGISwapChain *swapChain;
		factory->CreateSwapChain(_renderer->GetCommandQueue(), &swapChainDesc, &swapChain);
		_swapChain = static_cast<IDXGISwapChain3 *>(swapChain);
		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		_framebuffer = new D3D12Framebuffer(size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);
	}

	Vector2 D3D12SwapChain::GetSize() const
	{
		return _size;
	}

	ID3D12Resource *D3D12SwapChain::GetD3D12Buffer(int i) const
	{
		ID3D12Resource *bufferResource;
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&bufferResource));

		return bufferResource;
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

	void D3D12SwapChain::Prepare(D3D12CommandList *commandList)
	{
		// Indicate that the back buffer will be used as a render target.
		ID3D12Resource *renderTarget = GetFramebuffer()->GetSwapChainColorBuffer();
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	void D3D12SwapChain::Finalize(D3D12CommandList *commandList)
	{
		ID3D12Resource *renderTarget = GetFramebuffer()->GetSwapChainColorBuffer();
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}

	void D3D12SwapChain::PresentBackBuffer()
	{
		_swapChain->Present(0, 0); //First parameter is related to vsync
	}
}
