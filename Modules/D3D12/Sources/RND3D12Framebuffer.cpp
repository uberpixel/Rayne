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
#include "RND3D12Internals.h"

namespace RN
{
	RNDefineMeta(D3D12Framebuffer, Framebuffer)

	static D3D12Framebuffer::D3D12ColorTargetView *D3D12ColorTargetViewFromTargetView(const Framebuffer::TargetView &targetView)
	{
		D3D12Framebuffer::D3D12ColorTargetView *colorTargetView = new D3D12Framebuffer::D3D12ColorTargetView();
		colorTargetView->targetView = targetView;
		colorTargetView->targetView.texture->Retain();
		colorTargetView->d3dTargetViewDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(targetView.texture->GetDescriptor().format);

		//TODO: Support multisampled array render targets and plane slices
		switch(targetView.texture->GetDescriptor().type)
		{
		case Texture::Type::Type1D:
		{
			colorTargetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			colorTargetView->d3dTargetViewDesc.Texture1D.MipSlice = targetView.mipmap;
			break;
		}
			
		case Texture::Type::Type1DArray:
		{
			colorTargetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
			colorTargetView->d3dTargetViewDesc.Texture1DArray.MipSlice = targetView.mipmap;
			colorTargetView->d3dTargetViewDesc.Texture1DArray.FirstArraySlice = targetView.slice;
			colorTargetView->d3dTargetViewDesc.Texture1DArray.ArraySize = targetView.length;
			break;
		}

		case Texture::Type::Type2D:
		{
			colorTargetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			colorTargetView->d3dTargetViewDesc.Texture2D.MipSlice = targetView.mipmap;
			colorTargetView->d3dTargetViewDesc.Texture2D.PlaneSlice = 0;
			break;
		}

		case Texture::Type::Type2DMS:
		{
			colorTargetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			break;
		}

		case Texture::Type::Type2DArray:
		{
			colorTargetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			colorTargetView->d3dTargetViewDesc.Texture2DArray.MipSlice = targetView.mipmap;
			colorTargetView->d3dTargetViewDesc.Texture2DArray.PlaneSlice = 0;
			colorTargetView->d3dTargetViewDesc.Texture2DArray.FirstArraySlice = targetView.slice;
			colorTargetView->d3dTargetViewDesc.Texture2DArray.ArraySize = targetView.length;
			break;
		}

		case Texture::Type::Type3D:
		{
			colorTargetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			colorTargetView->d3dTargetViewDesc.Texture3D.MipSlice = targetView.mipmap;
			colorTargetView->d3dTargetViewDesc.Texture3D.FirstWSlice = targetView.slice;
			colorTargetView->d3dTargetViewDesc.Texture3D.WSize = targetView.length;
			break;
		}

		default:
			RN_ASSERT(false, "Unsupported render target type ");
		}

		return colorTargetView;
	}

	static D3D12Framebuffer::D3D12DepthStencilTargetView *D3D12DepthStencilTargetViewFromTargetView(const Framebuffer::TargetView &targetView)
	{
		D3D12Framebuffer::D3D12DepthStencilTargetView *depthStencilTargetView = new D3D12Framebuffer::D3D12DepthStencilTargetView();
		depthStencilTargetView->targetView = targetView;
		depthStencilTargetView->targetView.texture->Retain();
		depthStencilTargetView->d3dTargetViewDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(targetView.texture->GetDescriptor().format);

		//TODO: Support multisampled array render targets
		switch (targetView.texture->GetDescriptor().type)
		{
		case Texture::Type::Type1D:
		{
			depthStencilTargetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			depthStencilTargetView->d3dTargetViewDesc.Texture1D.MipSlice = targetView.mipmap;
			break;
		}

		case Texture::Type::Type1DArray:
		{
			depthStencilTargetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
			depthStencilTargetView->d3dTargetViewDesc.Texture1DArray.MipSlice = targetView.mipmap;
			depthStencilTargetView->d3dTargetViewDesc.Texture1DArray.FirstArraySlice = targetView.slice;
			depthStencilTargetView->d3dTargetViewDesc.Texture1DArray.ArraySize = targetView.length;
			break;
		}

		case Texture::Type::Type2D:
		{
			depthStencilTargetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencilTargetView->d3dTargetViewDesc.Texture2D.MipSlice = targetView.mipmap;
			break;
		}

		case Texture::Type::Type2DMS:
		{
			depthStencilTargetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
			break;
		}

		case Texture::Type::Type2DArray:
		{
			depthStencilTargetView->d3dTargetViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			depthStencilTargetView->d3dTargetViewDesc.Texture2DArray.MipSlice = targetView.mipmap;
			depthStencilTargetView->d3dTargetViewDesc.Texture2DArray.FirstArraySlice = targetView.slice;
			depthStencilTargetView->d3dTargetViewDesc.Texture2DArray.ArraySize = targetView.length;
			break;
		}

		default:
			RN_ASSERT(false, "Unsupported depth stencil target type ");
		}

		return depthStencilTargetView;
	}

	D3D12Framebuffer::D3D12Framebuffer(const Vector2 &size, D3D12SwapChain *swapChain, D3D12Renderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat) :
		Framebuffer(Vector2()),
		_renderer(renderer),
		_swapChain(swapChain),
		_swapChainColorBuffers(nullptr),
		_depthStencilTarget(nullptr),
		_rtvHandle(nullptr),
		_dsvHandle(nullptr)
	{
		DidUpdateSwapChain(size, colorFormat, depthStencilFormat);
	}

	D3D12Framebuffer::D3D12Framebuffer(const Vector2 &size, D3D12Renderer *renderer) :
		Framebuffer(size),
		_renderer(renderer),
		_swapChain(nullptr),
		_swapChainColorBuffers(nullptr),
		_depthStencilTarget(nullptr),
		_rtvHandle(nullptr),
		_dsvHandle(nullptr)
	{
/*		_colorFormat = D3D12ImageFormatFromTextureFormat(descriptor.colorFormat);
		_depthFormat = D3D12ImageFormatFromTextureFormat(descriptor.depthFormat);

		if (descriptor.colorFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor colorDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.colorFormat, size.x, size.y, false);
			colorDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;
			Texture *colorTexture = Texture::WithDescriptor(colorDescriptor);
			SetColorTexture(colorTexture);
		}

		if (descriptor.depthFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.depthFormat, size.x, size.y, false);
			depthDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;
			Texture *depthTexture = Texture::WithDescriptor(depthDescriptor);
			SetDepthTexture(depthTexture);
		}*/
	}

	D3D12Framebuffer::~D3D12Framebuffer()
	{
		//TODO: Maybe release swap chain resources!?

		if(_swapChainColorBuffers)
			delete[] _swapChainColorBuffers;

		for(D3D12ColorTargetView *targetView : _colorTargets)
		{
			targetView->targetView.texture->Release();
			delete targetView;
		}

		if(_depthStencilTarget)
		{
			_depthStencilTarget->targetView.texture->Release();
			delete _depthStencilTarget;
		}

		if (_rtvHandle)
			delete _rtvHandle;
		if (_dsvHandle)
			delete _dsvHandle;
	}

	void D3D12Framebuffer::SetColorTarget(const TargetView &target, uint32 index)
	{
		RN_ASSERT(!_swapChain, "A swap chain framebuffer can not have additional color targets!");
		RN_ASSERT(target.texture, "The color target needs a texture!");
		target.texture->Retain();

		D3D12ColorTargetView *targetView = D3D12ColorTargetViewFromTargetView(target);
		if(index < _colorTargets.size())
		{
			_colorTargets[index]->targetView.texture->Release();
			delete _colorTargets[index];
			_colorTargets[index] = targetView;
		}
		else
		{
			_colorTargets.push_back(targetView);
		}
	}

	void D3D12Framebuffer::SetDepthStencilTarget(const TargetView &target)
	{
		RN_ASSERT(target.texture, "The depth stencil target needs a texture!");
		target.texture->Retain();

		D3D12DepthStencilTargetView *targetView = D3D12DepthStencilTargetViewFromTargetView(target);
		if(_depthStencilTarget)
		{
			_depthStencilTarget->targetView.texture->Release();
			delete _depthStencilTarget;
		}
		
		_depthStencilTarget = targetView;
	}

	Texture *D3D12Framebuffer::GetColorTexture(uint32 index) const
	{
		if(index >= _colorTargets.size())
			return nullptr;

		return _colorTargets[index]->targetView.texture;
	}

	Texture *D3D12Framebuffer::GetDepthStencilTexture() const
	{
		if (!_depthStencilTarget)
			return nullptr;

		return _depthStencilTarget->targetView.texture;
	}

	ID3D12Resource *D3D12Framebuffer::GetSwapChainColorBuffer() const
	{
		RN_ASSERT(_swapChain, "GetSwapChainColorBuffer should only be called if there is a swap chain associated with the framebuffer.");
		if(_swapChain)
		{
			return _swapChainColorBuffers[_swapChain->GetFrameIndex()];
		}

		return nullptr;
	}

	void D3D12Framebuffer::PrepareAsRendertargetForFrame(uint32 frame)
	{
		ID3D12Device *device = _renderer->GetD3D12Device()->GetDevice();
		_frameLastUsed = frame;

		if(_rtvHandle)
			delete _rtvHandle;
		if(_dsvHandle)
			delete _dsvHandle;

		if(_colorTargets.size() > 0)
		{
			//TODO: Create heaps per framebuffer and not per camera
			D3D12DescriptorHeap *rtvHeap = _renderer->GetDescriptorHeap(_colorTargets.size(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
			_renderer->SubmitDescriptorHeap(rtvHeap);

			//Create the render target view
			uint32 counter = 0;
			if(_swapChain)
			{
				device->CreateRenderTargetView(_swapChainColorBuffers[_swapChain->GetFrameIndex()], &_colorTargets[0]->d3dTargetViewDesc, rtvHeap->GetCPUHandle(counter++));
			}
			else
			{
				for(D3D12ColorTargetView *targetView : _colorTargets)
				{
					device->CreateRenderTargetView(targetView->targetView.texture->Downcast<D3D12Texture>()->_resource, &targetView->d3dTargetViewDesc, rtvHeap->GetCPUHandle(counter++));
				}
			}

			_rtvHandle = new CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUHandle(0));
		}

		if(_depthStencilTarget)
		{
			//TODO: Create heaps per framebuffer and not per camera
			D3D12DescriptorHeap *dsvHeap = _renderer->GetDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
			_renderer->SubmitDescriptorHeap(dsvHeap);

			device->CreateDepthStencilView(_depthStencilTarget->targetView.texture->Downcast<D3D12Texture>()->_resource, &_depthStencilTarget->d3dTargetViewDesc, dsvHeap->GetCPUHandle(0));
			_dsvHandle = new CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUHandle(0));
		}
	}

	void D3D12Framebuffer::SetAsRendertarget(D3D12CommandList *commandList) const
	{
		//Set the rendertargets
		commandList->GetCommandList()->OMSetRenderTargets(_colorTargets.size() ? 1 : 0, _rtvHandle, false, _dsvHandle);
	}

	void D3D12Framebuffer::ClearColorTargets(D3D12CommandList *commandList, const Color &color)
	{
		if(_colorTargets.size() == 0)
			return;

		D3D12_RECT clearRect{ 0, 0, static_cast<LONG>(GetSize().x), static_cast<LONG>(GetSize().y) };
		commandList->GetCommandList()->ClearRenderTargetView(*_rtvHandle, &color.r, 1, &clearRect);
	}

	void D3D12Framebuffer::ClearDepthStencilTarget(D3D12CommandList *commandList, float depth, uint8 stencil)
	{
		if(!_depthStencilTarget)
			return;

		D3D12_RECT clearRect{ 0, 0, static_cast<LONG>(GetSize().x), static_cast<LONG>(GetSize().y) };
		//TODO: Needs D3D12_CLEAR_FLAG_STENCIL to also clear stencil buffer
		commandList->GetCommandList()->ClearDepthStencilView(*_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 1, &clearRect);
	}

	void D3D12Framebuffer::WillUpdateSwapChain()
	{
		if(_swapChainColorBuffers)
		{
			uint8 bufferCount = _swapChain->GetBufferCount();
			for(uint8 i = 0; i < bufferCount; i++)
			{
				_swapChainColorBuffers[i]->Release();
			}

			delete[] _swapChainColorBuffers;
			_swapChainColorBuffers = nullptr;
		}
	}

	void D3D12Framebuffer::DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat)
	{
		_size = size;

		for(D3D12ColorTargetView *targetView : _colorTargets)
		{
			delete targetView;
		}
		_colorTargets.clear();

		uint8 bufferCount = _swapChain->GetBufferCount();
		if(bufferCount > 0)
		{
			_swapChainColorBuffers = new ID3D12Resource*[bufferCount];

			for(uint8 i = 0; i < bufferCount; i++)
			{
				_swapChainColorBuffers[i] = _swapChain->GetD3D12Buffer(i);
			}
		}

		D3D12ColorTargetView *targetView = new D3D12ColorTargetView();
		targetView->targetView.texture = nullptr;
		targetView->targetView.mipmap = 0;
		targetView->targetView.slice = 0;
		targetView->targetView.length = 1;
		targetView->d3dTargetViewDesc.Format = D3D12Texture::ImageFormatFromTextureFormat(colorFormat);
		targetView->d3dTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		targetView->d3dTargetViewDesc.Texture2D.MipSlice = 0;
		targetView->d3dTargetViewDesc.Texture2D.PlaneSlice = 0;
		_colorTargets.push_back(targetView);

		if(depthStencilFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DRenderTargetFormat(depthStencilFormat, size.x, size.y);

			TargetView target;
			target.texture = Texture::WithDescriptor(depthDescriptor);
			target.mipmap = 0;
			target.slice = 0;
			target.length = 1;
			SetDepthStencilTarget(target);
		}
	}
}
