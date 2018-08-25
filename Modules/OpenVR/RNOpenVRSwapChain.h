//
//  RNOpenVRSwapChain.h
//  Rayne-OpenVR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRSWAPCHAIN_H_
#define __RAYNE_OPENVRSWAPCHAIN_H_

#include "RNOpenVR.h"
#include <openvr.h>

struct ID3D12Resource;
namespace RN
{
	class D3D12CommandList;
	class OpenVRSwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRSwapChain();

		OVRAPI virtual void UpdatePredictedPose();

		OVRAPI virtual void ResizeOpenVRSwapChain(const Vector2 &size) = 0;
		OVRAPI virtual Vector2 GetOpenVRSwapChainSize() const = 0;
		OVRAPI virtual const Window::SwapChainDescriptor &GetOpenVRSwapChainDescriptor() const = 0;
		OVRAPI virtual Framebuffer *GetOpenVRSwapChainFramebuffer() const = 0;

	protected:
		OpenVRSwapChain(vr::IVRSystem *system);

		Texture *_targetTexture;
		Vector3 _hmdToEyeViewOffset[2];

		vr::IVRSystem *_vrSystem;
		vr::TrackedDevicePose_t _frameDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _predictedDevicePose[vr::k_unMaxTrackedDeviceCount];

		static const uint32 kEyePadding;
	};
}


#endif /* __RAYNE_OPENVRSWAPCHAIN_H_ */
