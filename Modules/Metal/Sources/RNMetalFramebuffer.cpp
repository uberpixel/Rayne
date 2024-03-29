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
		_sampleCount(1),
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
		_sampleCount(1),
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
		RN_ASSERT((_colorTargets.size() == 0 && !_depthStencilTarget) || _sampleCount == target.texture->GetDescriptor().sampleCount, "Texture sample count differs from other framebuffer textures sample count");
		
		target.texture->Retain();
		_sampleCount = target.texture->GetDescriptor().sampleCount;
		
		id<MTLTexture> mtlTexture = static_cast< id<MTLTexture> >(target.texture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
		MetalTargetView *newTarget = new MetalTargetView;
		newTarget->targetView = target;
		newTarget->pixelFormat = [mtlTexture pixelFormat];

		if(index < _colorTargets.size())
		{
			_colorTargets[index]->targetView.texture->Release();
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
		RN_ASSERT(_colorTargets.size() == 0 || _sampleCount == target.texture->GetDescriptor().sampleCount, "Texture sample count differs from other framebuffer textures sample count");
		
		target.texture->Retain();
		_sampleCount = target.texture->GetDescriptor().sampleCount;

		id<MTLTexture> mtlTexture = static_cast< id<MTLTexture> >(target.texture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
		MetalTargetView *newTarget = new MetalTargetView;
		newTarget->targetView = target;
		newTarget->pixelFormat = [mtlTexture pixelFormat];

		if(_depthStencilTarget)
		{
			_depthStencilTarget->targetView.texture->Release();
			delete _depthStencilTarget;
		}

		_depthStencilTarget = newTarget;
	}

	void MetalFramebuffer::SetSwapchainDepthStencilTarget(const TargetView &target, Texture::Format colorFormat)
	{
		MetalTargetView *newTarget = new MetalTargetView;
		newTarget->targetView = target;
		newTarget->pixelFormat = MetalTexture::PixelFormatForTextureFormat(colorFormat);

		if(_depthStencilTarget)
		{
			_depthStencilTarget->targetView.texture->Release();
			delete _depthStencilTarget;
		}

		_depthStencilTarget = newTarget;
	}

	Texture *MetalFramebuffer::GetColorTexture(uint32 index) const
	{
		if(index >= _colorTargets.size())
			return nullptr;

		return _colorTargets[index]->targetView.texture;
	}

	Texture *MetalFramebuffer::GetDepthStencilTexture() const
	{
		if(!_depthStencilTarget)
			return nullptr;

		return _depthStencilTarget->targetView.texture;
	}
	
	uint8 MetalFramebuffer::GetSampleCount() const
	{
		return _sampleCount;
	}

	MTLRenderPassDescriptor *MetalFramebuffer::GetRenderPassDescriptor(RenderPass *renderPass, MetalFramebuffer *resolveFramebuffer, uint8 multiviewLayer, uint8 multiviewCount) const
	{
		//TODO: Currently the next camera into the same framebuffer will clear the whole framebuffer...
		//There does not appear to be a way to only clear part of the framebuffer...
		const Color &clearColor = renderPass->GetClearColor();
		MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];

		int counter = 0;
		for(MetalTargetView *metalTarget : _colorTargets)
		{
			const TargetView &target = metalTarget->targetView;
			id<MTLTexture> texture = nil;
			if(!target.texture)
			{
				RN_ASSERT(_swapChain, "Empty color target view texture, but no swapchain!");
				texture = _swapChain->GetMetalColorTexture();
			}
			else
			{
				texture = static_cast<id<MTLTexture>>(static_cast<MetalTexture *>(target.texture)->__GetUnderlyingTexture());
			}

			MTLRenderPassColorAttachmentDescriptor *colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:counter];
			[colorAttachment setTexture:texture];
			if(renderPass->GetFlags() & RenderPass::Flags::ClearColor)
			{
				[colorAttachment setLoadAction:MTLLoadActionClear];
			}
			else if(renderPass->GetFlags() & RenderPass::Flags::LoadColor)
			{
				[colorAttachment setLoadAction:MTLLoadActionLoad];
			}
			else
			{
				[colorAttachment setLoadAction:MTLLoadActionDontCare];
			}
			if(resolveFramebuffer)
			{
				[colorAttachment setStoreAction:MTLStoreActionMultisampleResolve];
				
				if(resolveFramebuffer->GetColorTexture())
				{
					[colorAttachment setResolveTexture:static_cast< id<MTLTexture> >(resolveFramebuffer->GetColorTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture())];
				}
				else if(resolveFramebuffer->GetSwapChain())
				{
					[colorAttachment setResolveTexture:static_cast< id<MTLTexture> >(resolveFramebuffer->GetSwapChain()->GetMetalColorTexture())];
				}
			}
			else
			{
				if(renderPass->GetFlags() & RenderPass::Flags::StoreColor)
				{
					[colorAttachment setStoreAction:MTLStoreActionStore];
				}
				else
				{
					[colorAttachment setStoreAction:MTLStoreActionDontCare];
				}
			}
			[colorAttachment setClearColor:MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a)];
			
			if(multiviewCount > 0 || multiviewLayer > 0)
			{
				[colorAttachment setSlice:multiviewLayer];
			}
			else
			{
				[colorAttachment setSlice:metalTarget->targetView.slice];
			}
			[colorAttachment setLevel:metalTarget->targetView.mipmap];
			
			counter += 1;
		}

		if(_depthStencilTarget)
		{
			id<MTLTexture> depthStencilTexture = nil;
			if(!_depthStencilTarget->targetView.texture)
			{
				RN_ASSERT(_swapChain, "Empty depth target view texture, but no swapchain!");
				depthStencilTexture = _swapChain->GetMetalDepthTexture();
			}
			else
			{
				depthStencilTexture = static_cast<id<MTLTexture>>(static_cast<MetalTexture *>(_depthStencilTarget->targetView.texture)->__GetUnderlyingTexture());
			}

			//TODO: Improve check
			if(
#if RN_PLATFORM_MAC_OS
			   [depthStencilTexture pixelFormat] != MTLPixelFormatX24_Stencil8 &&
#endif
			   [depthStencilTexture pixelFormat] != MTLPixelFormatX32_Stencil8)
			{
				MTLRenderPassDepthAttachmentDescriptor *depthAttachment = [descriptor depthAttachment];
				[depthAttachment setTexture:depthStencilTexture];
				if(renderPass->GetFlags() & RenderPass::Flags::ClearDepthStencil)
				{
					[depthAttachment setLoadAction:MTLLoadActionClear];
					[depthAttachment setClearDepth:renderPass->GetClearDepth()];
				}
				else if(renderPass->GetFlags() & RenderPass::Flags::LoadDepthStencil)
				{
					[depthAttachment setLoadAction:MTLLoadActionLoad];
				}
				else
				{
					[depthAttachment setLoadAction:MTLLoadActionDontCare];
				}
				if(resolveFramebuffer && resolveFramebuffer->_depthStencilTarget)
				{
					[depthAttachment setStoreAction:MTLStoreActionMultisampleResolve];
					if(resolveFramebuffer->GetDepthStencilTexture())
					{
						[depthAttachment setResolveTexture:static_cast< id<MTLTexture> >(resolveFramebuffer->GetDepthStencilTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture())];
					}
					else if(resolveFramebuffer->GetSwapChain())
					{
						[depthAttachment setResolveTexture:static_cast< id<MTLTexture> >(resolveFramebuffer->GetSwapChain()->GetMetalDepthTexture())];
					}
				}
				else
				{
					if(renderPass->GetFlags() & RenderPass::Flags::StoreDepthStencil)
					{
						[depthAttachment setStoreAction:MTLStoreActionStore];
					}
					else
					{
						[depthAttachment setStoreAction:MTLStoreActionDontCare];
					}
				}
				
				if(multiviewCount > 0 || multiviewLayer > 0)
				{
					[depthAttachment setSlice:multiviewLayer];
				}
				else
				{
					[depthAttachment setSlice:_depthStencilTarget->targetView.slice];
				}
				
				[depthAttachment setLevel:_depthStencilTarget->targetView.mipmap];
			}

			//TODO: Improve check
			if(
#if RN_PLATFORM_MAC_OS
			   [depthStencilTexture pixelFormat] == MTLPixelFormatDepth24Unorm_Stencil8 || [depthStencilTexture pixelFormat] == MTLPixelFormatX24_Stencil8 ||
#endif
			   [depthStencilTexture pixelFormat] == MTLPixelFormatDepth32Float_Stencil8 || [depthStencilTexture pixelFormat] == MTLPixelFormatX32_Stencil8)
			{
				MTLRenderPassStencilAttachmentDescriptor *stencilAttachment = [descriptor stencilAttachment];
				[stencilAttachment setTexture:depthStencilTexture];
				
				if(renderPass->GetFlags() & RenderPass::Flags::ClearDepthStencil)
				{
					[stencilAttachment setLoadAction:MTLLoadActionClear];
					[stencilAttachment setClearStencil:renderPass->GetClearStencil()];
				}
				else if(renderPass->GetFlags() & RenderPass::Flags::LoadDepthStencil)
				{
					[stencilAttachment setLoadAction:MTLLoadActionLoad];
				}
				else
				{
					[stencilAttachment setLoadAction:MTLLoadActionDontCare];
				}
				
				if(resolveFramebuffer && resolveFramebuffer->GetDepthStencilTexture())
				{
					[stencilAttachment setStoreAction:MTLStoreActionMultisampleResolve];
					if(resolveFramebuffer->GetDepthStencilTexture())
					{
						[stencilAttachment setResolveTexture:static_cast< id<MTLTexture> >(resolveFramebuffer->GetDepthStencilTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture())];
					}
					else if(resolveFramebuffer->GetSwapChain())
					{
						[stencilAttachment setResolveTexture:static_cast< id<MTLTexture> >(resolveFramebuffer->GetSwapChain()->GetMetalDepthTexture())];
					}
				}
				else
				{
					if(renderPass->GetFlags() & RenderPass::Flags::StoreDepthStencil)
					{
						[stencilAttachment setStoreAction:MTLStoreActionStore];
					}
					else
					{
						[stencilAttachment setStoreAction:MTLStoreActionDontCare];
					}
				}
				
				if(multiviewCount > 0 || multiviewLayer > 0)
				{
					[stencilAttachment setSlice:multiviewLayer];
				}
				else
				{
					[stencilAttachment setSlice:_depthStencilTarget->targetView.slice];
				}
				
				[stencilAttachment setLevel:_depthStencilTarget->targetView.mipmap];
			}
		}

		return descriptor;
	}

	void MetalFramebuffer::DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat)
	{
		_size = size;

		for(MetalTargetView *targetView : _colorTargets)
		{
			delete targetView;
		}
		_colorTargets.clear();

		MetalTargetView *colorTarget = new MetalTargetView;
		colorTarget->targetView.texture = nullptr;
		colorTarget->targetView.mipmap = 0;
		colorTarget->targetView.slice = 0;
		colorTarget->targetView.length = 1;
		colorTarget->pixelFormat = MetalTexture::PixelFormatForTextureFormat(colorFormat);
		_colorTargets.push_back(colorTarget);

		if(depthStencilFormat != Texture::Format::Invalid)
		{
			TargetView target;
			target.mipmap = 0;
			target.slice = 0;
			target.length = 1;
			
			Texture::Descriptor depthDescriptor = Texture::Descriptor::With2DRenderTargetFormat(depthStencilFormat, size.x, size.y);
			target.texture = Texture::WithDescriptor(depthDescriptor);
			
			SetDepthStencilTarget(target);
		}
	}

	void MetalFramebuffer::DidUpdateSwapChainSize(Vector2 size)
	{
		_size = size;
	}
	
	MTLPixelFormat MetalFramebuffer::GetMetalColorFormat(uint8 texture) const
	{
		if(_colorTargets.size() > 0)//texture)
		{
			return _colorTargets[/*texture*/0]->pixelFormat;
		}
		
		return MTLPixelFormatInvalid;
	}
	
 	MTLPixelFormat MetalFramebuffer::GetMetalDepthFormat() const
	{
		if(_depthStencilTarget)
		{
			MTLPixelFormat depthStencilFormat = _depthStencilTarget->pixelFormat;
			if(
#if RN_PLATFORM_MAC_OS
			   depthStencilFormat != MTLPixelFormatX24_Stencil8 &&
#endif
			   depthStencilFormat != MTLPixelFormatX32_Stencil8)
			{
				return depthStencilFormat;
			}
		}
		
		return MTLPixelFormatInvalid;
	}
	
 	MTLPixelFormat MetalFramebuffer::GetMetalStencilFormat() const
	{
		if(_depthStencilTarget)
		{
			MTLPixelFormat depthStencilFormat = _depthStencilTarget->pixelFormat;
			if(
#if RN_PLATFORM_MAC_OS
			   depthStencilFormat == MTLPixelFormatX24_Stencil8 || depthStencilFormat == MTLPixelFormatDepth24Unorm_Stencil8 ||
#endif
			   depthStencilFormat == MTLPixelFormatX32_Stencil8 || depthStencilFormat == MTLPixelFormatDepth32Float_Stencil8)
			{
				return depthStencilFormat;
			}
		}
		
		return MTLPixelFormatInvalid;
	}
}
