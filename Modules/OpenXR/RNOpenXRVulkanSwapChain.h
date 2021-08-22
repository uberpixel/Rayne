//
//  RNOpenXRVulkanSwapChain.h
//  Rayne-OpenXR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OpenXRVULKANSWAPCHAIN_H_
#define __RAYNE_OpenXRVULKANSWAPCHAIN_H_

#include "RNVulkanRenderer.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanSwapChain.h"

#include "openxr/openxr.h"

#include "RNOpenXR.h"

namespace RN
{
	class OpenXRWindow;
	struct OpenXRSwapchainInternals;
	class OpenXRVulkanSwapChain : public VulkanSwapChain
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

	private:
		OpenXRVulkanSwapChain(const OpenXRWindow *window, const Window::SwapChainDescriptor &descriptor, const Vector2 &size);
		void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic);

		const OpenXRWindow *_window;
		OpenXRSwapchainInternals *_internals;

		VkImage *_swapchainImages;
		VkImage *_swapchainFoveationImages;
		Vector2 *_swapChainFoveationImagesSize;

		std::function<void()> _presentEvent;

		RNDeclareMetaAPI(OpenXRVulkanSwapChain, OXRAPI)
	};
}


#endif /* __RAYNE_OpenXRVULKANSWAPCHAIN_H_ */
