//
//  RND3D12Framebuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Framebuffer.h"
#include "RND3D12Renderer.h"
#include "RND3D12SwapChain.h"

namespace RN
{
	RNDefineMeta(D3D12Framebuffer, Framebuffer)

	D3D12Framebuffer::D3D12Framebuffer(const Vector2 &size, const Descriptor &descriptor, D3D12SwapChain *swapChain, D3D12Renderer *renderer) :
		Framebuffer(size, descriptor),
		_renderer(renderer),
		_swapChain(swapChain),
		_renderTargets(nullptr),
		_depthStencilBuffer(nullptr)
	{
		ID3D12Device *device = renderer->GetD3D12Device()->GetDevice();

		_renderTargets = new ID3D12Resource*[swapChain->GetBufferCount()];

		for(int i = 0; i < swapChain->GetBufferCount(); i++)
		{
			_renderTargets[i] = swapChain->GetD3D12Buffer(i);
		}

		if(descriptor.depthFormat != Texture::Format::Invalid)
		{
			// Create depthbuffer
			D3D12_CLEAR_VALUE depthClearValue = {};
			depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthClearValue.DepthStencil.Depth = 1.0f;
			depthClearValue.DepthStencil.Stencil = 0;
			device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, size.x, size.y, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&_depthStencilBuffer));
		}
	}

	D3D12Framebuffer::~D3D12Framebuffer()
	{
		//TODO: Release render target d3d resources, might not be needed for swap chain buffers though, not sure

		if(_renderTargets)
			delete _renderTargets;

		if(_depthStencilBuffer)
			_depthStencilBuffer->Release();
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

	ID3D12Resource *D3D12Framebuffer::GetRenderTarget() const
	{
		if(_swapChain)
		{
			return _renderTargets[_swapChain->GetFrameIndex()];
		}

		return _renderTargets[0];
	}
}
