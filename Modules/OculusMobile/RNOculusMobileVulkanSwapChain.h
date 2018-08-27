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

		OVRAPI void UpdatePredictedPose();

	private:
		OculusMobileVulkanSwapChain(const Window::SwapChainDescriptor &descriptor, ovrJava java);
		const String *GetHMDInfoDescription() const;
		OVRAPI void SetProjection(float m22, float m23, float m32);
		ovrMatrix4f GetTanAngleMatrixForEye(uint8 eye);

		void CreateSharedVulkanImage();

		void UpdateVRMode();

		int _mainThreadID;
		ovrJava _java;
		ovrTextureSwapChain *_colorSwapChain;
		ovrMobile *_session;
		uint32 _actualFrameIndex;
		double _predictedDisplayTime;
		ovrTracking2 _hmdState;
		Vector2 _eyeRenderSize;

		ANativeWindow *_nativeWindow;

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OculusMobileVulkanSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSMOBILEVULKANSWAPCHAIN_H_ */
