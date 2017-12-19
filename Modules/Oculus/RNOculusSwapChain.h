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

		OVRAPI ID3D12Resource *GetD3D12ColorBuffer(int i) const final;
		OVRAPI ID3D12Resource *GetD3D12DepthBuffer(int i) const final;

		OVRAPI void UpdatePredictedPose();

		bool HasDepthBuffer() const final { return _depthSwapChain; }
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; };

	private:
		OculusSwapChain(const Window::SwapChainDescriptor &descriptor);
		const String *GetHMDInfoDescription() const;
		OVRAPI void SetProjection(float m22, float m23, float m32);

		ovrSession _session;
		ovrGraphicsLuid _luID;
		ovrHmdDesc _hmdDescription;
		ovrLayerEyeFovDepth _imageLayer;
		ovrTextureSwapChain _colorSwapChain;
		ovrTextureSwapChain _depthSwapChain;

		ovrEyeRenderDesc _eyeRenderDesc[2];
		ovrPosef _hmdToEyeViewPose[2];
		ovrTrackingState _hmdState;

		ovrResult _submitResult;

		long long _frameCounter;

		static const uint32 kEyePadding;

		RNDeclareMetaAPI(OculusSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSSWAPCHAIN_H_ */
