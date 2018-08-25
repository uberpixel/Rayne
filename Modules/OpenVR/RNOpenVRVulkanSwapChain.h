//
//  RNOpenVRVulkanSwapChain.h
//  Rayne-OpenVR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRVULKANSWAPCHAIN_H_
#define __RAYNE_OPENVRVULKANSWAPCHAIN_H_

#include "RNOpenVRSwapChain.h"
#include "RNVulkanSwapChain.h"

namespace RN
{
	class OpenVRVulkanSwapChain : public VulkanSwapChain, OpenVRSwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRVulkanSwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(VkCommandBuffer commandBuffer) final;
		OVRAPI void Finalize(VkCommandBuffer commandBuffer) final;
		OVRAPI void PresentBackBuffer(VkQueue queue) final;

		OVRAPI VkImage GetVulkanColorBuffer(int i) const final;

		OVRAPI void ResizeOpenVRSwapChain(const Vector2 &size) final;
		OVRAPI Vector2 GetOpenVRSwapChainSize() const final { return GetSize(); }
		OVRAPI const Window::SwapChainDescriptor &GetOpenVRSwapChainDescriptor() const final { return _descriptor; }
		OVRAPI Framebuffer *GetOpenVRSwapChainFramebuffer() const final;

	protected:
		OpenVRVulkanSwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system);

	private:
		bool _isFirstRender;

		RNDeclareMetaAPI(OpenVRVulkanSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRVULKANSWAPCHAIN_H_ */
