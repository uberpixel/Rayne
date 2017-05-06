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
		struct TargetView
		{
			static TargetView WithTexture(Texture *texture, uint16 mipmap = 0, uint16 slice = 0, uint16 length = 1)
			{
				TargetView targetView;
				targetView.texture = texture;
				targetView.mipmap = mipmap;
				targetView.slice = slice;
				targetView.length = length;
				return targetView;
			}

			Texture *texture;
			uint16 mipmap;
			uint16 slice;
			uint16 length;
		};

		const Vector2 &GetSize() const { return _size; }

		RNAPI virtual Texture *GetColorTexture(uint32 index = 0) const = 0;
		RNAPI virtual Texture *GetDepthStencilTexture() const = 0;

		RNAPI virtual void SetColorTarget(const TargetView &target, uint32 index = 0) = 0;
		RNAPI virtual void SetDepthStencilTarget(const TargetView &target) = 0;

	protected:
		RNAPI Framebuffer(const Vector2 &size);
		RNAPI ~Framebuffer();

	private:
		Vector2 _size;

		__RNDeclareMetaInternal(Framebuffer)
	};
}


#endif /* __RAYNE_FRAMEBUFFER_H_ */
