//
//  RNOpenVRVulkanSwapChain.h
//  Rayne-OpenVR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRVULKANSWAPCHAIN_H_
#define __RAYNE_OPENVRVULKANSWAPCHAIN_H_

#include "RNVulkanSwapChain.h"

#include "RNOpenVR.h"
#include <openvr.h>

namespace RN
{
	class OpenVRVulkanSwapChain : public VulkanSwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRVulkanSwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(VkCommandBuffer commandBuffer) final;
		OVRAPI void Finalize(VkCommandBuffer commandBuffer) final;
		OVRAPI void PresentBackBuffer(VkQueue queue) final;

		OVRAPI VkImage GetVulkanColorBuffer(int i) const final;
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; }

		OVRAPI void UpdatePredictedPose();

	protected:
		OpenVRVulkanSwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system);

	private:
		void ResizeSwapchain(const Vector2 &size);

		Texture *_targetTexture;
		Vector3 _hmdToEyeViewOffset[2];
		bool _isFirstRender;

		vr::IVRSystem *_vrSystem;
		vr::TrackedDevicePose_t _frameDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _predictedDevicePose[vr::k_unMaxTrackedDeviceCount];

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OpenVRVulkanSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRVULKANSWAPCHAIN_H_ */
