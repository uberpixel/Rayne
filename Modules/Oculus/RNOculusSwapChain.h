//
//  RNOculusSwapChain.h
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSSWAPCHAIN_H_
#define __RAYNE_OCULUSSWAPCHAIN_H_

#include "RND3D12Renderer.h"
#include "RND3D12Framebuffer.h"
#include "RND3D12SwapChain.h"

#include "OVR_CAPI_D3D.h"

#include "RNOculus.h"

namespace RN
{
	class OculusSwapChain : public D3D12SwapChain
	{
	public:
		friend class OculusWindow;

		OVRAPI ~OculusSwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(D3D12CommandList *commandList) final;
		OVRAPI void Finalize(D3D12CommandList *commandList) final;
		OVRAPI void PresentBackBuffer() final;

		OVRAPI ID3D12Resource *GetD3D12Buffer(int i) const final;

		OVRAPI void UpdatePredictedPose();

	private:
		OculusSwapChain();
		const String *GetHMDInfoDescription() const;

		ovrSession _session;
		ovrGraphicsLuid _luID;
		ovrHmdDesc _hmdDescription;
		ovrLayerEyeFov _layer;
		ovrTextureSwapChain _textureSwapChain;

		ovrEyeRenderDesc _eyeRenderDesc[2];
		ovrVector3f _hmdToEyeViewOffset[2];
		ovrTrackingState _hmdState;

		RNDeclareMetaAPI(OculusSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSSWAPCHAIN_H_ */
