//
//  RNOpenXRMetalSwapChain.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_OpenXRMETALSWAPCHAIN_H_
#define __RAYNE_OpenXRMETALSWAPCHAIN_H_

#include "RNMetalRenderer.h"
#include "RNMetalFramebuffer.h"
#include "RNMetalSwapChain.h"

#include "RNOpenXR.h"
#include "RNOpenXRSwapChain.h"

namespace RN
{
	struct OpenXRSwapchainInternals;
	class OpenXRCompositorLayer;
	class OpenXRMetalSwapChain : public MetalSwapChain, public OpenXRSwapChain
	{
	public:
		friend class OpenXRWindow;
		friend OpenXRCompositorLayer;

		OXRAPI ~OpenXRMetalSwapChain();

		OXRAPI void AcquireBackBuffer() final;
		OXRAPI void Prepare() final;
		OXRAPI void Finalize() final;
		OXRAPI void PresentBackBuffer(id<MTLCommandBuffer> commandBuffer) final;

		OXRAPI id GetMetalColorTexture() const final;
		OXRAPI id GetMetalDepthTexture() const final;

		OXRAPI void ResizeSwapChain(const Vector2 &size) final;
		Vector2 GetSwapChainSize() const final { return GetSize(); }
		const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final { return _descriptor; }
		Framebuffer *GetSwapChainFramebuffer() const final;
		OXRAPI void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) final;

	private:
		OpenXRMetalSwapChain(const OpenXRWindow *window, OpenXRCompositorLayer *layer, const Window::SwapChainDescriptor &descriptor, const Vector2 &size);

		void **_swapchainImages;
		//VkImage *_swapchainFoveationImages;
		//Vector2 *_swapChainFoveationImagesSize;

		RNDeclareMetaAPI(OpenXRMetalSwapChain, OXRAPI)
	};
}

#endif /* __RAYNE_OpenXRMETALSWAPCHAIN_H_ */
