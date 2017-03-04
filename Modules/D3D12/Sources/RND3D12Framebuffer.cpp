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

		// Create depthbuffer
		D3D12_CLEAR_VALUE depthClearValue = {};
		depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;
		device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, size.x, size.y, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&_depthStencilBuffer));

		// Create depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		device->CreateDepthStencilView(_depthStencilBuffer, &depthStencilViewDesc, renderer->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart());
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
