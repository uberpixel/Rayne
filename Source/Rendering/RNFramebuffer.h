//
//  RNFramebuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_FRAMEBUFFER_H_
#define __RAYNE_FRAMEBUFFER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNTexture.h"

namespace RN
{
	class Framebuffer : public Object
	{
	public:
		RN_OPTIONS(Options, uint32,
				PrivateStorage = (1 << 0));

		struct Descriptor
		{
			Descriptor() :
				options(0),
				colorFormat(Texture::Format::RGBA8888),
				depthFormat(Texture::Format::Invalid),
				stencilFormat(Texture::Format::Invalid)
			{}

			Descriptor(const Descriptor &) = default;
			Descriptor &operator =(const Descriptor &) = default;

			Options options;
			Texture::Format colorFormat;
			Texture::Format depthFormat;
			Texture::Format stencilFormat;
		};

		const Descriptor &GetDescriptor() const { return _descriptor; }
		const Vector2 &GetSize() const { return _size; }

		RNAPI virtual Texture *GetColorTexture() const = 0;
		RNAPI virtual Texture *GetDepthTexture() const = 0;
		RNAPI virtual Texture *GetStencilTexture() const = 0;

	protected:
		RNAPI Framebuffer(const Vector2 &size, const Descriptor &descriptor);
		RNAPI ~Framebuffer();

	private:
		Vector2 _size;
		Descriptor _descriptor;

		__RNDeclareMetaInternal(Framebuffer)
	};
}


#endif /* __RAYNE_FRAMEBUFFER_H_ */
