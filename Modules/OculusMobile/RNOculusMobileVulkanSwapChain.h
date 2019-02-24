//
//  RNOculusMobileVulkanSwapChain.h
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_
#define __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_

#include "RNVulkanRenderer.h"
#include "RNVulkanFramebuffer.h"
#include "RNVulkanSwapChain.h"

#include "VrApi.h"

#include "RNOculusMobile.h"

namespace RN
{
	class OculusMobileVulkanSwapChain : public VulkanSwapChain
	{
	public:
		friend class OculusMobileWindow;

		OVRAPI ~OculusMobileVulkanSwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(VkCommandBuffer commandBuffer) final;
		OVRAPI void Finalize(VkCommandBuffer commandBuffer) final;
		OVRAPI void PresentBackBuffer(VkQueue queue) final;

		OVRAPI VkImage GetVulkanColorBuffer(int i) const final;
		OVRAPI VkImage GetVulkanDepthBuffer(int i) const final;

	private:
		OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor, const Vector2 &size);
		ovrMatrix4f GetTanAngleMatrixForEye(uint8 eye);

		ovrTracking2 _hmdState;

		ovrTextureSwapChain *_colorSwapChain;

		std::function<void()> _presentEvent;

		RNDeclareMetaAPI(OculusMobileVulkanSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_ */
