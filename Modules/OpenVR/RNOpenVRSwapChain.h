//
//  RNOpenVRSwapChain.h
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRSWAPCHAIN_H_
#define __RAYNE_OPENVRSWAPCHAIN_H_

#include "RND3D12Renderer.h"
#include "RND3D12SwapChain.h"
#include "RND3D12Framebuffer.h"

#include <openvr.h>
#include "RNOpenVR.h"

namespace RN
{
	class OpenVRSwapChain : public D3D12SwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRSwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(D3D12CommandList *commandList) final;
		OVRAPI void Finalize(D3D12CommandList *commandList) final;
		OVRAPI void PresentBackBuffer() final;

		OVRAPI ID3D12Resource *GetD3D12Buffer(int i) const final;

		OVRAPI void UpdatePredictedPose();

	protected:
		OpenVRSwapChain(const Window::SwapChainDescriptor &descriptor);

	private:
		const String *GetHMDInfoDescription() const;

		Texture *_targetTexture;
		Vector3 _hmdToEyeViewOffset[2];
		bool _isFirstRender;

		vr::IVRSystem *_hmd;
		vr::TrackedDevicePose_t _frameDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _predictedDevicePose[vr::k_unMaxTrackedDeviceCount];

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OpenVRSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRSWAPCHAIN_H_ */
