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

		VKAPI void AcquireBackBuffer() final;
		VKAPI void Prepare(VkCommandBuffer commandBuffer) final;
		VKAPI void Finalize(VkCommandBuffer commandBuffer) final;
		VKAPI void PresentBackBuffer(VkQueue queue) final;

		VKAPI VkImage GetVulkanColorBuffer(int i) const final;
		VKAPI VkImage GetVulkanDepthBuffer(int i) const final;

		OVRAPI void UpdatePredictedPose();
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; }

	private:
		OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor);
		const String *GetHMDInfoDescription() const;
		OVRAPI void SetProjection(float m22, float m23, float m32);

		ovrJava _java;
		ovrTextureSwapChain *_colorSwapChain;

/*		ovrSession _session;
		ovrGraphicsLuid _luID;
		ovrHmdDesc _hmdDescription;
		ovrLayerEyeFovDepth _imageLayer;
		ovrTextureSwapChain _colorSwapChain;
		ovrTextureSwapChain _depthSwapChain;

		ovrEyeRenderDesc _eyeRenderDesc[2];
		ovrPosef _hmdToEyeViewPose[2];
		ovrTrackingState _hmdState;

		ovrResult _submitResult;*/

		long long _frameCounter;

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OculusMobileVulkanSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_ */
