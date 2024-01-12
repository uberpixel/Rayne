//
//  RNAppleXRMetalSwapChain.h
//  Rayne-AppleXR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_APPLEXRMETALSWAPCHAIN_H_
#define __RAYNE_APPLEXRMETALSWAPCHAIN_H_

#include "RNMetalRenderer.h"
#include "RNMetalSwapChain.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalDevice.h"
#include "RNAppleXRSwapChain.h"

#include "RNAppleXR.h"

namespace RN
{
	class AppleXRMetalSwapChain : public MetalSwapChain, AppleXRSwapChain
	{
	public:
		friend class AppleXRWindow;

		AXRAPI ~AppleXRMetalSwapChain();
		
		AXRAPI void AcquireBackBuffer() final;
		AXRAPI void Prepare() final;
		AXRAPI void Finalize() final;
		AXRAPI void PresentBackBuffer(id<MTLCommandBuffer> commandBuffer) final;
		AXRAPI void PostPresent(id<MTLCommandBuffer> commandBuffer) final;
		
		AXRAPI id GetMetalColorTexture() const final;
		AXRAPI id GetMetalDepthTexture() const final;
		
		AXRAPI Vector2 GetAppleXRSwapChainSize() const final { return GetSize(); };
		AXRAPI Framebuffer *GetAppleXRSwapChainFramebuffer() const final;
		
		const Window::SwapChainDescriptor &GetAppleXRSwapChainDescriptor() const final { return _descriptor; }

	protected:
		AXRAPI AppleXRMetalSwapChain(const Window::SwapChainDescriptor &descriptor, cp_layer_renderer_t layerRenderer);

	private:
		cp_drawable_t _drawable;

		RNDeclareMetaAPI(AppleXRMetalSwapChain, AXRAPI)
	};
}


#endif /* __RAYNE_APPLEXRMETALSWAPCHAIN_H_ */
