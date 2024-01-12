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

#if RN_PLATFORM_MAC_OS
@class RNMetalView;
#endif
#if RN_PLATFORM_IOS
class RNMetalLayerContainer;
#endif
namespace RN
{
	class MetalFramebuffer;
	class MetalSwapChain : public Object
	{
	public:
		friend class MetalWindow;

		MTLAPI ~MetalSwapChain();

		MTLAPI Vector2 GetSize() const;

		MTLAPI virtual void SetFrameDivider(uint8 divider);
		MTLAPI virtual void AcquireBackBuffer();
		MTLAPI virtual void Prepare();
		MTLAPI virtual void Finalize();
		MTLAPI virtual void PresentBackBuffer(id<MTLCommandBuffer> commandBuffer);
		MTLAPI virtual void PostPresent(id<MTLCommandBuffer> commandBuffer);

		MTLAPI virtual id GetMetalColorTexture() const;
		MTLAPI virtual id GetMetalDepthTexture() const;
		
		MetalFramebuffer *GetFramebuffer() const { return _framebuffer; }
		
		const Window::SwapChainDescriptor &GetSwapChainDescriptor() const { return _descriptor; }

		uint8 GetBufferCount() const { return 4; } //TODO: Return something better!?
		
	protected:
		MTLAPI MetalSwapChain(){}
		
		MetalFramebuffer *_framebuffer;
		size_t _frameIndex;
		Window::SwapChainDescriptor _descriptor;
		
		Vector2 _size;
		Vector2 _newSize;
		uint8 _frameDivider;

	private:
#if RN_PLATFORM_MAC_OS
		MetalSwapChain(const Vector2 size, id<MTLDevice> device, Screen *screen, const Window::SwapChainDescriptor &descriptor);
		RNMetalView *_metalView;
#endif
#if RN_PLATFORM_IOS
		MetalSwapChain(const Vector2 size, RNMetalLayerContainer *metalLayerContainer, const Window::SwapChainDescriptor &descriptor);
		RNMetalLayerContainer *_metalLayerContainer;
#endif

		id _drawable;

		RNDeclareMetaAPI(MetalSwapChain, MTLAPI)
	};
}

#endif //__RAYNE_METALSWAPCHAIN_H
