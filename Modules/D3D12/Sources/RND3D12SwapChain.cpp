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

	static DXGI_FORMAT SwapChainFormatFromTextureFormat(Texture::Format format)
	{
		switch(format)
		{
		case Texture::Format::RGBA_8_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::BGRA_8_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::Format::RGBA_8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::BGRA_8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Texture::Format::RGB_10_A_2:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Texture::Format::RGBA_16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}


	D3D12SwapChain::D3D12SwapChain(const Vector2& size, HWND hwnd, D3D12Renderer* renderer, const Window::SwapChainDescriptor& descriptor) :
		_renderer(renderer),
		_frameIndex(0),
		_size(size),
		_descriptor(descriptor),
		_swapChain(nullptr),
		_framebuffer(nullptr),
		_fenceValues(nullptr)
	{
		_fenceValues = new UINT[descriptor.bufferCount];
		for (int i = 0; i < descriptor.bufferCount; i++)
			_fenceValues[i] = 0;

		ID3D12Device* device = _renderer->GetD3D12Device()->GetDevice();
		device->CreateFence(_fenceValues[_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		_fenceValues[_frameIndex] ++;
		_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferCount = _descriptor.bufferCount;
		swapChainDesc.BufferDesc.Width = size.x;
		swapChainDesc.BufferDesc.Height = size.y;
		swapChainDesc.BufferDesc.Format = SwapChainFormatFromTextureFormat(descriptor.colorFormat);
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Windowed = !descriptor.wantsFullscreen;

		IDXGISwapChain* swapChain;
		IDXGIFactory4* factory = _renderer->GetD3D12Descriptor()->GetFactory();
		factory->CreateSwapChain(_renderer->GetCommandQueue(), &swapChainDesc, &swapChain);
		_swapChain = static_cast<IDXGISwapChain3 *>(swapChain);
		_framebuffer = new D3D12Framebuffer(size, this, _renderer, _descriptor.colorFormat, _descriptor.depthStencilFormat);
	}

	D3D12SwapChain::~D3D12SwapChain()
	{
		delete[] _fenceValues;
	}

	void D3D12SwapChain::ResizeSwapchain(const Vector2& size)
	{
		_newSize = size;
	}

	void D3D12SwapChain::SetFullscreen(bool fullscreen) const
	{
		if(FAILED(_swapChain->SetFullscreenState(!fullscreen, nullptr)))
		{
			// Transitions to fullscreen mode can fail when running apps over
			// terminal services or for some other unexpected reason.  Consider
			// notifying the user in some way when this happens.
			RN_ASSERT(false, "Failed changing from/to fullscreen.");
		}
	}

	Vector2 D3D12SwapChain::GetSize() const
	{
		return _size;
	}

	ID3D12Resource* D3D12SwapChain::GetD3D12ColorBuffer(int i) const
	{
		ID3D12Resource* bufferResource;
		_swapChain->GetBuffer(i, IID_PPV_ARGS(&bufferResource));

		return bufferResource;
	}

	ID3D12Resource* D3D12SwapChain::GetD3D12DepthBuffer(int i) const
	{
		return nullptr;
	}

	void D3D12SwapChain::AcquireBackBuffer()
	{
		const UINT64 fenceValue = _fenceValues[_frameIndex];
		_renderer->GetCommandQueue()->Signal(_fence, fenceValue);

		//Update the swap chain and frame buffer size if the window size changed
		if(_newSize.GetLength() > 0.001f)
		{
			if(_size.GetSquaredDistance(_newSize) > 0.001f)
			{
				_size = _newSize;

				//Wait for rendering to all currently bound buffers is finished
				UINT completedFenceValue = _fence->GetCompletedValue();
				if(completedFenceValue < _fenceValues[_frameIndex])
				{
					_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
					WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
				}

				//Do the actual resize and notify the framebuffer about it
				_framebuffer->WillUpdateSwapChain();
				_swapChain->ResizeBuffers(_descriptor.bufferCount, _size.x, _size.y, SwapChainFormatFromTextureFormat(_descriptor.colorFormat), 0);
				_framebuffer->DidUpdateSwapChain(_size, _descriptor.colorFormat, _descriptor.depthStencilFormat);

				_frameIndex = _swapChain->GetCurrentBackBufferIndex();
			}

			_newSize = Vector2();
		}

		_frameIndex = _swapChain->GetCurrentBackBufferIndex();

		UINT completedFenceValue = _fence->GetCompletedValue();
		if(completedFenceValue < _fenceValues[_frameIndex])
		{
			_fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
			WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
		}

		_fenceValues[_frameIndex] = fenceValue + 1;
	}

	void D3D12SwapChain::Prepare(D3D12CommandList* commandList)
	{
		// Indicate that the back buffer will be used as a render target.
		ID3D12Resource* renderTarget = GetFramebuffer()->GetSwapChainColorBuffer();
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	void D3D12SwapChain::Finalize(D3D12CommandList* commandList)
	{
		ID3D12Resource* renderTarget = GetFramebuffer()->GetSwapChainColorBuffer();
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}

	void D3D12SwapChain::PresentBackBuffer()
	{
		_swapChain->Present(_descriptor.vsync, 0);
	}
}
