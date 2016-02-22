//
//  RNMetalFramebuffer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALFRAMEBUFFER_H_
#define __RAYNE_METALFRAMEBUFFER_H_

#include "RNMetal.h"

namespace RN
{
	class MetalFramebuffer : public Framebuffer
	{
	public:
		MTLAPI MetalFramebuffer(const Vector2 &size, const Descriptor &descriptor);
		MTLAPI ~MetalFramebuffer();

		MTLAPI Texture *GetColorTexture() const final;
		MTLAPI Texture *GetDepthTexture() const final;
		MTLAPI Texture *GetStencilTexture() const final;

	private:
		RNDeclareMetaAPI(MetalFramebuffer, MTLAPI)
	};
}


#endif /* __RAYNE_METALFRAMEBUFFER_H_ */
