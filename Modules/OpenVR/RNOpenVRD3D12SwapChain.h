//
//  RNOpenVRD3D12SwapChain.h
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRD3D12SWAPCHAIN_H_
#define __RAYNE_OPENVRD3D12SWAPCHAIN_H_

#include "RND3D12SwapChain.h"

#include "RNOpenVR.h"
#include <openvr.h>

struct ID3D12Resource;
namespace RN
{
	class D3D12CommandList;
	class OpenVRD3D12SwapChain : public D3D12SwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRD3D12SwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(D3D12CommandList *commandList) final;
		OVRAPI void Finalize(D3D12CommandList *commandList) final;
		OVRAPI void PresentBackBuffer() final;

		OVRAPI ID3D12Resource *GetD3D12Buffer(int i) const final;

		OVRAPI void UpdatePredictedPose();

	protected:
		OpenVRD3D12SwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system);

	private:
		void ResizeSwapchain(const Vector2 &size);

		Texture *_targetTexture;
		Vector3 _hmdToEyeViewOffset[2];
		bool _isFirstRender;

		vr::IVRSystem *_vrSystem;
		vr::TrackedDevicePose_t _frameDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _predictedDevicePose[vr::k_unMaxTrackedDeviceCount];

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OpenVRD3D12SwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRD3D12SWAPCHAIN_H_ */
