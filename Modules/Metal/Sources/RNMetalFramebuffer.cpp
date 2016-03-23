//
//  RNMetalFramebuffer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalFramebuffer.h"

namespace RN
{
	RNDefineMeta(MetalFramebuffer, Framebuffer)

	MetalFramebuffer::MetalFramebuffer(const Vector2 &size, const Descriptor &descriptor) :
		Framebuffer(size, descriptor),
		_colorTexture(nullptr),
		_depthTexture(nullptr),
		_stencilTexture(nullptr)
	{
		if(descriptor.options & Options::PrivateStorage)
		{
			Renderer *renderer = Renderer::GetActiveRenderer();

			if(descriptor.colorFormat != Texture::Format::Invalid)
			{
				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.colorFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_colorTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);
			}

			if(descriptor.depthFormat != Texture::Format::Invalid)
			{
				switch(descriptor.depthFormat)
				{
					case Texture::Format::Depth24I:
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32F:
					case Texture::Format::Depth32FStencil8:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid depth format");
				}


				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.depthFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_depthTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);

				if(descriptor.stencilFormat == descriptor.depthFormat)
					_stencilTexture = _depthTexture->Retain();
			}

			if(descriptor.stencilFormat != Texture::Format::Invalid && descriptor.stencilFormat != descriptor.depthFormat)
			{
				switch(descriptor.depthFormat)
				{
					case Texture::Format::Stencil8:
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32FStencil8:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid stencil format");
				}


				Texture::Descriptor textureDescriptor = Texture::Descriptor::With2DTextureAndFormat(descriptor.stencilFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
				textureDescriptor.usageHint |= Texture::Descriptor::UsageHint::RenderTarget;

				_stencilTexture = renderer->CreateTextureWithDescriptor(textureDescriptor);
			}
		}
	}

	MetalFramebuffer::~MetalFramebuffer()
	{
		SafeRelease(_colorTexture);
		SafeRelease(_depthTexture);
		SafeRelease(_stencilTexture);
	}

	Texture *MetalFramebuffer::GetColorTexture() const
	{
		if(GetDescriptor().options & Options::PrivateStorage)
			return _colorTexture;

		throw InvalidCallException("GetColorTexture() can only be called on private storage framebuffer");
	}

	Texture *MetalFramebuffer::GetDepthTexture() const
	{
		if(GetDescriptor().options & Options::PrivateStorage)
			return _depthTexture;

		throw InvalidCallException("GetDepthTexture() can only be called on private storage framebuffer");
	}

	Texture *MetalFramebuffer::GetStencilTexture() const
	{
		if(GetDescriptor().options & Options::PrivateStorage)
			return _stencilTexture;

		throw InvalidCallException("GetStencilTexture() can only be called on private storage framebuffer");
	}
}
