//
//  RNOpenXRVulkanSwapChain.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_OpenXRVULKANSWAPCHAIN_H_
#define __RAYNE_OpenXRVULKANSWAPCHAIN_H_

#include "RNVulkanRenderer.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanSwapChain.h"

#include "RNOpenXR.h"
#include "RNOpenXRSwapChain.h"

namespace RN
{
	class OpenXRWindow;
	struct OpenXRSwapchainInternals;
	class OpenXRVulkanSwapChain : public VulkanSwapChain, OpenXRSwapChain
	{
	public:
		friend OpenXRWindow;

		OXRAPI ~OpenXRVulkanSwapChain();

		OXRAPI void AcquireBackBuffer() final;
		OXRAPI void Prepare(VkCommandBuffer commandBuffer) final;
		OXRAPI void Finalize(VkCommandBuffer commandBuffer) final;
		OXRAPI void PresentBackBuffer(VkQueue queue) final;

		OXRAPI VkImage GetVulkanColorBuffer(int i) const final;
		OXRAPI VkImage GetVulkanDepthBuffer(int i) const final;
		OXRAPI VkImage GetVulkanFragmentDensityBuffer(int i, uint32 &width, uint32 &height) const final;

		OXRAPI void ResizeSwapChain(const Vector2 &size) final;
		Vector2 GetSwapChainSize() const final { return GetSize(); }
		const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final { return _descriptor; }
		Framebuffer *GetSwapChainFramebuffer() const final;
		OXRAPI void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) final;

	private:
		OpenXRVulkanSwapChain(const OpenXRWindow *window, const Window::SwapChainDescriptor &descriptor, const Vector2 &size);

		VkImage *_swapchainImages;
		VkImage *_swapchainFoveationImages;
		Vector2 *_swapChainFoveationImagesSize;

		RNDeclareMetaAPI(OpenXRVulkanSwapChain, OXRAPI)
	};
}


#endif /* __RAYNE_OpenXRVULKANSWAPCHAIN_H_ */
