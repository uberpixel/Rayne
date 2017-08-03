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
		MTLAPI MetalFramebuffer(const Vector2 &size, MetalSwapChain *swapChain, Texture::Format colorFormat, Texture::Format depthStencilFormat);
		MTLAPI MetalFramebuffer(const Vector2 &size);
		MTLAPI ~MetalFramebuffer();

		MTLAPI void SetColorTarget(const TargetView &target, uint32 index = 0) final;
		MTLAPI void SetDepthStencilTarget(const TargetView &target) final;

		MTLAPI Texture *GetColorTexture(uint32 index = 0) const final;
		MTLAPI Texture *GetDepthStencilTexture() const final;

		MetalSwapChain *GetSwapChain() const { return _swapChain; }
		id<MTLTexture> GetRenderTarget() const;

	private:
		void DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat);

		std::vector<TargetView *> _colorTargets;
		TargetView *_depthStencilTarget;

		WeakRef<MetalSwapChain> _swapChain;

		RNDeclareMetaAPI(MetalFramebuffer, MTLAPI)
	};
}


#endif /* __RAYNE_METALFRAMEBUFFER_H_ */
