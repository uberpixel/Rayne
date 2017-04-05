//
//  RNMetalSwapChain.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALSWAPCHAIN_H
#define __RAYNE_METALSWAPCHAIN_H

#include "RNMetal.h"

@class RNMetalView;
namespace RN
{
	class MetalFramebuffer;
	class MetalSwapChain : public Object
	{
	public:
		friend class MetalWindow;

		MTLAPI ~MetalSwapChain();

		MTLAPI Vector2 GetSize() const;

		MTLAPI virtual void AcquireBackBuffer();
		MTLAPI virtual void Prepare();
		MTLAPI virtual void Finalize();
		MTLAPI virtual void PresentBackBuffer(id<MTLCommandBuffer> commandBuffer);

		MTLAPI virtual id GetMetalDrawable() const;
		MetalFramebuffer *GetFramebuffer() const { return _framebuffer; }

	private:
		MetalSwapChain(const Vector2 size, id<MTLDevice> device);

		RNMetalView *_metalView;
		MetalFramebuffer *_framebuffer;
		id _drawable;

		RNDeclareMetaAPI(MetalSwapChain, MTLAPI)
	};
}

#endif //__RAYNE_METALSWAPCHAIN_H
