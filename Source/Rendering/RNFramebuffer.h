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

		RNAPI Framebuffer(const Vector2 &size, Options options, Texture::Format colorFormat, Texture::Format depthFormat = Texture::Format::Invalid, Texture::Format stencilFormat = Texture::Format::Invalid);

		Options GetOptions() const { return _options; }
		const Vector2 &GetSize() const { return _size; }

		RNAPI Texture *GetColorTexture() const;
		RNAPI Texture *GetDepthTexture() const;
		RNAPI Texture *GetStencilTexture() const;

	private:
		Vector2 _size;
		Options _options;

		Texture *_colorTexture;
		Texture *_depthTexture;
		Texture *_stencilTexture;

		RNDeclareMeta(Framebuffer)
	};
}


#endif /* __RAYNE_FRAMEBUFFER_H_ */
