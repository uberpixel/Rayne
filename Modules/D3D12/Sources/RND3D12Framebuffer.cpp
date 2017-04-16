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

	static DXGI_FORMAT D3D12ImageFormatFromTextureFormat(Texture::Format format)
	{
		switch (format)
		{
		case Texture::Format::RGBA8888SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Texture::Format::RGBA8888:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::RGB10A2:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Texture::Format::R8:
			return DXGI_FORMAT_R8_UNORM;
		case Texture::Format::RG88:
			return DXGI_FORMAT_R8G8_UNORM;
		case Texture::Format::RGB888:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texture::Format::R16F:
			return DXGI_FORMAT_R16_FLOAT;
		case Texture::Format::RG16F:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Texture::Format::RGB16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::Format::RGBA16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Texture::Format::R32F:
			return DXGI_FORMAT_R32_FLOAT;
		case Texture::Format::RG32F:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Texture::Format::RGB32F:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Texture::Format::RGBA32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Texture::Format::Depth24I:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::Format::Depth32F:
			return DXGI_FORMAT_D32_FLOAT;
		case Texture::Format::Stencil8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::Format::Depth24Stencil8:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Texture::Format::Depth32FStencil8:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	D3D12Framebuffer::D3D12Framebuffer(const Vector2 &size, const Descriptor &descriptor, D3D12SwapChain *swapChain, D3D12Renderer *renderer) :
		Framebuffer(size, descriptor),
		_renderer(renderer),
		_swapChain(swapChain),
		_swapChainColorBuffers(nullptr),
		_colorTexture(nullptr),
		_depthTexture(nullptr),
		_stencilTexture(nullptr),
		_colorDimension(D3D12_RTV_DIMENSION_TEXTURE2D),
		_depthDimension(D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		_colorFormat = D3D12ImageFormatFromTextureFormat(descriptor.colorFormat);
		_depthFormat = D3D12ImageFormatFromTextureFormat(descriptor.depthFormat);

		_swapChainColorBuffers = new ID3D12Resource*[swapChain->GetBufferCount()];

		for(int i = 0; i < swapChain->GetBufferCount(); i++)
		{
			_swapChainColorBuffers[i] = swapChain->GetD3D12Buffer(i);
		}

		if(descriptor.depthFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.depthFormat, size.x, size.y, false);
			depthDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;
			Texture *depthTexture = Texture::WithDescriptor(depthDescriptor);
			SetDepthTexture(depthTexture);
		}
	}

	D3D12Framebuffer::~D3D12Framebuffer()
	{
		//TODO: Maybe release swap chain resources!?

		if(_swapChainColorBuffers)
			delete[] _swapChainColorBuffers;

		SafeRelease(_colorTexture);
		SafeRelease(_depthTexture);
		SafeRelease(_stencilTexture);
	}

	void D3D12Framebuffer::SetColorTexture(Texture *texture)
	{
		//TODO: Handle multiple textures
		RN_ASSERT(_swapChain, "The color texture of a swap chain framebuffer can not be changed!");
		SafeRelease(_colorTexture);
		_colorTexture = texture->Downcast<D3D12Texture>()->Retain();
	}

	void D3D12Framebuffer::SetDepthTexture(Texture *texture)
	{
		SafeRelease(_depthTexture);
		_depthTexture = texture->Downcast<D3D12Texture>()->Retain();
	}

	void D3D12Framebuffer::SetStencilTexture(Texture *texture)
	{
		//TODO: Handle shared depth/stencil textures
		SafeRelease(_stencilTexture);
		_stencilTexture = texture->Downcast<D3D12Texture>()->Retain();
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

	ID3D12Resource *D3D12Framebuffer::GetColorBuffer() const
	{
		if(_swapChain)
		{
			return _swapChainColorBuffers[_swapChain->GetFrameIndex()];
		}

		return _swapChainColorBuffers[0];
	}

	ID3D12Resource* D3D12Framebuffer::GetDepthBuffer() const
	{
		return _depthTexture->_resource;
	}
}
