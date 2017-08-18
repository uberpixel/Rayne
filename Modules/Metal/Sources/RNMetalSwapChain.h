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

		MTLAPI virtual id GetMTLTexture() const;
		MetalFramebuffer *GetFramebuffer() const { return _framebuffer; }

		uint8 GetBufferCount() const { return 4; } //TODO: Return something better!?
		
	protected:
		MTLAPI MetalSwapChain(){}
		
		MetalFramebuffer *_framebuffer;
		size_t _frameIndex;
		Window::SwapChainDescriptor _descriptor;
		
		Vector2 _size;
		Vector2 _newSize;

	private:
		MetalSwapChain(const Vector2 size, id<MTLDevice> device, const Window::SwapChainDescriptor &descriptor);

		RNMetalView *_metalView;
		id _drawable;

		RNDeclareMetaAPI(MetalSwapChain, MTLAPI)
	};
}

#endif //__RAYNE_METALSWAPCHAIN_H
