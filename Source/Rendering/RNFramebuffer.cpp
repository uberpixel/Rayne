//
//  RNFramebuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFramebuffer.h"
#include "RNRenderer.h"

namespace RN
{
	RNDefineMeta(Framebuffer, Object)

	Framebuffer::Framebuffer(const Vector2 &size, Options options, Texture::Format colorFormat, Texture::Format depthFormat, Texture::Format stencilFormat) :
		_size(size),
		_options(options),
		_colorTexture(nullptr),
		_depthTexture(nullptr),
		_stencilTexture(nullptr)
	{
		if(options & Options::PrivateStorage)
		{
			Renderer *renderer = Renderer::GetActiveRenderer();

			if(colorFormat != Texture::Format::Invalid)
			{
				Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(colorFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				_colorTexture = renderer->CreateTextureWithDescriptor(descriptor);
			}

			if(depthFormat != Texture::Format::Invalid)
			{
				switch(depthFormat)
				{
					case Texture::Format::Depth24I:
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32F:
					case Texture::Format::Depth32FStencil8:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid depth format");
				}


				Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(depthFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				_depthTexture = renderer->CreateTextureWithDescriptor(descriptor);

				if(stencilFormat == depthFormat)
					_stencilTexture = _depthTexture->Retain();
			}

			if(stencilFormat != Texture::Format::Invalid && stencilFormat != depthFormat)
			{
				switch(depthFormat)
				{
					case Texture::Format::Stencil8:
					case Texture::Format::Depth24Stencil8:
					case Texture::Format::Depth32FStencil8:
						break;

					default:
						throw InvalidArgumentException("Framebuffer() called with invalid stencil format");
				}


				Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(stencilFormat, static_cast<uint32>(size.x), static_cast<uint32>(size.y), false);
				_stencilTexture = renderer->CreateTextureWithDescriptor(descriptor);
			}
		}
	}

	Texture *Framebuffer::GetColorTexture() const
	{
		if(_options & Options::PrivateStorage)
			return _colorTexture;

		throw InvalidCallException("GetColorTexture() can only be called on private storage framebuffer");
	}

	Texture *Framebuffer::GetDepthTexture() const
	{
		if(_options & Options::PrivateStorage)
			return _depthTexture;

		throw InvalidCallException("GetDepthTexture() can only be called on private storage framebuffer");
	}

	Texture *Framebuffer::GetStencilTexture() const
	{
		if(_options & Options::PrivateStorage)
			return _stencilTexture;

		throw InvalidCallException("GetStencilTexture() can only be called on private storage framebuffer");
	}
}
