//
//  RND3D12Framebuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Framebuffer.h"
#include "RND3D12Renderer.h"

namespace RN
{
	RNDefineMeta(D3D12Framebuffer, Framebuffer)

	D3D12Framebuffer::D3D12Framebuffer(const Vector2 &size, const Descriptor &descriptor, IDXGISwapChain3 *swapChain, D3D12Renderer *renderer) :
		Framebuffer(size, descriptor),
		_renderer(renderer)
	{
		ID3D12Device *device = renderer->GetD3D12Device()->GetDevice();

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderer->GetRTVHeap()->GetCPUDescriptorHandleForHeapStart());

		for(int i = 0; i < 3; i++)
		{
			swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i]));
			device->CreateRenderTargetView(_renderTargets[i], nullptr, rtvHandle);
			rtvHandle.Offset(1, renderer->GetRTVHeapSize());
		}
	}

	D3D12Framebuffer::~D3D12Framebuffer()
	{

	}

	Texture *D3D12Framebuffer::GetColorTexture() const
	{
		return nullptr;
	}
	Texture *D3D12Framebuffer::GetDepthTexture() const
	{
		return nullptr;
	}
	Texture *D3D12Framebuffer::GetStencilTexture() const
	{
		return nullptr;
	}
}
