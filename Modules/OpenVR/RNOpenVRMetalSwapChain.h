//
//  RNOpenVRMetalSwapChain.h
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRMETALSWAPCHAIN_H_
#define __RAYNE_OPENVRMETALSWAPCHAIN_H_

#include "RNMetalRenderer.h"
#include "RNMetalSwapChain.h"
#include "RNMetalFramebuffer.h"

#include <openvr.h>
#include "RNOpenVR.h"

namespace RN
{
	class OpenVRMetalSwapChain : public MetalSwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRMetalSwapChain();
		
		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare() final;
		OVRAPI void Finalize() final;
		OVRAPI void PresentBackBuffer(id<MTLCommandBuffer> commandBuffer) final;
		OVRAPI void PostPresent(id<MTLCommandBuffer> commandBuffer) final;
		
		OVRAPI id GetMTLTexture() const final;

		OVRAPI void UpdatePredictedPose();

	protected:
		OVRAPI OpenVRMetalSwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system);

	private:
		void ResizeSwapchain(const Vector2 &size);

		MetalTexture *_targetTexture;
		IOSurfaceRef _targetSurface;
		
		Vector3 _hmdToEyeViewOffset[2];

		vr::IVRSystem *_vrSystem;
		vr::TrackedDevicePose_t _frameDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _predictedDevicePose[vr::k_unMaxTrackedDeviceCount];

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OpenVRMetalSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRMETALSWAPCHAIN_H_ */
