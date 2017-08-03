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

	MTLRenderPassDescriptor *MetalFramebuffer::GetRenderPassDescriptor(RenderPass *renderPass) const
	{
		//TODO: Currently the next camera into the same framebuffer will clear the whole framebuffer...
		//There does not appear to be a way to only clear part of the framebuffer...
		const Color &clearColor = renderPass->GetClearColor();
		MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];

		for(TargetView *target : _colorTargets)
		{
			id<MTLTexture> texture = nil;
			if(!target->texture)
			{
				RN_ASSERT(_swapChain, "Empty color target view texture, but no swapchain!");

				id<CAMetalDrawable> drawable = _swapChain->GetMetalDrawable();
				texture = [drawable texture];
			}
			else
			{
				texture = static_cast<id<MTLTexture>>(static_cast<MetalTexture *>(target->texture)->__GetUnderlyingTexture());
			}

			MTLRenderPassColorAttachmentDescriptor *colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:0];
			[colorAttachment setTexture:texture];
			[colorAttachment setLoadAction:MTLLoadActionClear];
			[colorAttachment setStoreAction:MTLStoreActionStore];
			[colorAttachment setClearColor:MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a)];
		}

		if(GetDepthStencilTexture())
		{
			id<MTLTexture> depthStencilTexture = static_cast< id<MTLTexture> >(GetDepthStencilTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture());

			//TODO: Check if texture format supports depth
			MTLRenderPassDepthAttachmentDescriptor *depthAttachment = [descriptor depthAttachment];
			[depthAttachment setTexture:depthStencilTexture];
			[depthAttachment setLoadAction:MTLLoadActionClear];
			[depthAttachment setStoreAction:MTLStoreActionStore];

			//TODO: Check if texture format supports stencil
/*			MTLRenderPassStencilAttachmentDescriptor *stencilAttachment = [descriptor stencilAttachment];
			[stencilAttachment setTexture:depthStencilTexture];
			[stencilAttachment setLoadAction:MTLLoadActionDontCare];
			[stencilAttachment setStoreAction:MTLStoreActionDontCare];*/
		}

		return descriptor;
	}

	void MetalFramebuffer::DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat)
	{
		_size = size;

		for(TargetView *targetView : _colorTargets)
		{
			delete targetView;
		}
		_colorTargets.clear();

		TargetView *colorTarget = new TargetView;
		colorTarget->texture = nullptr;
		colorTarget->mipmap = 0;
		colorTarget->slice = 0;
		colorTarget->length = 1;
		_colorTargets.push_back(colorTarget);

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
