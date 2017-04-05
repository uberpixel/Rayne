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
#include "RNMetalSwapChain.h"

namespace RN
{
	class MetalSwapChain;

	class MetalFramebuffer : public Framebuffer
	{
	public:
		MTLAPI MetalFramebuffer(const Vector2 &size, const Descriptor &descriptor, MetalSwapChain *swapChain);
		MTLAPI MetalFramebuffer(const Vector2 &size, const Descriptor &descriptor);
		MTLAPI ~MetalFramebuffer();

		MTLAPI Texture *GetColorTexture() const final;
		MTLAPI Texture *GetDepthTexture() const final;
		MTLAPI Texture *GetStencilTexture() const final;

		id<MTLTexture> GetRenderTarget() const;
		MetalSwapChain *GetSwapChain() const { return _swapChain; }

	private:
		Texture *_colorTexture;
		Texture *_depthTexture;
		Texture *_stencilTexture;

		WeakRef<MetalSwapChain> _swapChain;

		RNDeclareMetaAPI(MetalFramebuffer, MTLAPI)
	};
}


#endif /* __RAYNE_METALFRAMEBUFFER_H_ */
