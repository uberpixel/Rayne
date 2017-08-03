//
//  RNMetalFramebuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalFramebuffer.h"
#include "RNMetalSwapChain.h"
#include "RNMetalTexture.h"

#import <QuartzCore/QuartzCore.h>

namespace RN
{
	RNDefineMeta(MetalFramebuffer, Framebuffer)

	MetalFramebuffer::MetalFramebuffer(const Vector2 &size, MetalSwapChain *swapChain, Texture::Format colorFormat, Texture::Format depthStencilFormat) :
		Framebuffer(size),
		_depthStencilTarget(nullptr),
		_swapChain(swapChain)
	{
		DidUpdateSwapChain(size, colorFormat, depthStencilFormat);

/*		if(descriptor.options & Options::PrivateStorage)
		{
			Renderer *renderer = Renderer::GetActiveRenderer();
			Texture::Format stencilFormat = descriptor.stencilFormat;

			if(descriptor.depthFormat != Texture::Format::Invalid)
			{
				switch(descriptor.depthFormat)
				{
					case Texture::Format::Depth32FStencil8:
					case Texture::Format::Depth24Stencil8:
						stencilFormat = descriptor.depthFormat;
						break;
					case Texture::Format::Depth24I:
					case Texture::Format::Depth32F:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid depth format");
				}


				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.depthFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_depthTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);

				if(stencilFormat == descriptor.depthFormat)
					_stencilTexture = _depthTexture->Retain();
			}

			if(stencilFormat != Texture::Format::Invalid && stencilFormat != descriptor.depthFormat)
			{
				switch(stencilFormat)
				{
					case Texture::Format::Stencil8:
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32FStencil8:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid stencil format");
				}


				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(stencilFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_stencilTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);
			}
		}*/
	}

	MetalFramebuffer::MetalFramebuffer(const Vector2 &size) :
		Framebuffer(size),
		_depthStencilTarget(nullptr),
		_swapChain(nullptr)
	{
/*		if(descriptor.options & Options::PrivateStorage)
		{
			Renderer *renderer = Renderer::GetActiveRenderer();

			if(descriptor.colorFormat != Texture::Format::Invalid)
			{
				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.colorFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_colorTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);
			}
			
			Texture::Format stencilFormat = descriptor.stencilFormat;

			if(descriptor.depthFormat != Texture::Format::Invalid)
			{
				switch(descriptor.depthFormat)
				{
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32FStencil8:
						stencilFormat = descriptor.depthFormat;
						break;
					case Texture::Format::Depth24I:
					case Texture::Format::Depth32F:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid depth format");
				}


				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.depthFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_depthTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);

				if(stencilFormat == descriptor.depthFormat)
					_stencilTexture = _depthTexture->Retain();
			}

			if(stencilFormat != Texture::Format::Invalid && stencilFormat != descriptor.depthFormat)
			{
				switch(stencilFormat)
				{
					case Texture::Format::Stencil8:
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32FStencil8:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid stencil format");
				}


				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(stencilFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_stencilTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);
			}
		}*/
	}

	MetalFramebuffer::~MetalFramebuffer()
	{

	}

	void MetalFramebuffer::SetColorTarget(const TargetView &target, uint32 index)
	{
		RN_ASSERT(!_swapChain, "A swap chain framebuffer can not have additional color targets!");
		RN_ASSERT(target.texture, "The color target needs a texture!");
		RN_ASSERT(target.texture->GetDescriptor().accessOptions == GPUResource::AccessOptions::Private, "The framebuffer target needs to be in private storage");
		target.texture->Retain();

		TargetView *newTarget = new TargetView(target);

		if(index < _colorTargets.size())
		{
			_colorTargets[index]->texture->Release();
			delete _colorTargets[index];
			_colorTargets[index] = newTarget;
		}
		else
		{
			_colorTargets.push_back(newTarget);
		}
	}

	void MetalFramebuffer::SetDepthStencilTarget(const TargetView &target)
	{
		RN_ASSERT(target.texture, "The depth stencil target needs a texture!");
		RN_ASSERT(target.texture->GetDescriptor().accessOptions == GPUResource::AccessOptions::Private, "The framebuffer target needs to be in private storage");
		target.texture->Retain();

		TargetView *newTarget = new TargetView(target);

		if(_depthStencilTarget)
		{
			_depthStencilTarget->texture->Release();
			delete _depthStencilTarget;
		}

		_depthStencilTarget = newTarget;
	}

	Texture *MetalFramebuffer::GetColorTexture(uint32 index) const
	{
		if(index >= _colorTargets.size())
			return nullptr;

		return _colorTargets[index]->texture;
	}

	Texture *MetalFramebuffer::GetDepthStencilTexture() const
	{
		if(!_depthStencilTarget)
			return nullptr;

		return _depthStencilTarget->texture;
	}


	id<MTLTexture> MetalFramebuffer::GetRenderTarget() const
	{
		if(_swapChain)
		{
			id<CAMetalDrawable> drawable = _swapChain->GetMetalDrawable();
			return [drawable texture];
		}

		RN_ASSERT(_colorTargets.size() > 0, "The framebuffer has no color target!");
		return static_cast<id<MTLTexture>>(static_cast<MetalTexture *>(_colorTargets[0]->texture)->__GetUnderlyingTexture());
	}

	void MetalFramebuffer::DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat)
	{
		_size = size;

		for(TargetView *targetView : _colorTargets)
		{
			delete targetView;
		}
		_colorTargets.clear();


		if(depthStencilFormat != Texture::Format::Invalid)
		{
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DTextureAndFormat(depthStencilFormat, size.x, size.y, false);
			depthDescriptor.usageHint |= Texture::UsageHint::RenderTarget;

			TargetView target;
			target.texture = Texture::WithDescriptor(depthDescriptor);
			target.mipmap = 0;
			target.slice = 0;
			target.length = 1;
			SetDepthStencilTarget(target);
		}
	}
}
