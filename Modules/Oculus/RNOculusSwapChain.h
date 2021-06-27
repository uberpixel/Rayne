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

		bool HasDepthBuffer() const final { return _depthSwapChain[0]; }
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; }

	private:
		OculusSwapChain(const Window::SwapChainDescriptor &descriptor);
		const String *GetHMDInfoDescription() const;
		OVRAPI void SetProjection(float m22, float m23, float m32);

		ovrSession _session;
		ovrGraphicsLuid _luID;
		ovrHmdDesc _hmdDescription;
		ovrLayerEyeFovDepth _imageLayer;
		ovrTextureSwapChain _colorSwapChain[2];
		ovrTextureSwapChain _depthSwapChain[2];

		ovrEyeRenderDesc _eyeRenderDesc[2];
		ovrPosef _hmdToEyeViewPose[2];
		ovrTrackingState _hmdState;

		ovrResult _submitResult;

		D3D12Texture *_colorTexture;
		D3D12Texture *_depthTexture;
		ID3D12Resource **_swapChainColorBuffers[2];
		ID3D12Resource **_swapChainDepthBuffers[2];
		long long _frameCounter;

		RNDeclareMetaAPI(OculusSwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSSWAPCHAIN_H_ */
