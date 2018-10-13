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
#include "RNOpenVRSwapChain.h"

struct ID3D12Resource;
namespace RN
{
	class D3D12CommandList;
	class OpenVRD3D12SwapChain : public D3D12SwapChain, OpenVRSwapChain
	{
	public:
		friend class OpenVRWindow;

		OVRAPI ~OpenVRD3D12SwapChain();

		OVRAPI void AcquireBackBuffer() final;
		OVRAPI void Prepare(D3D12CommandList *commandList) final;
		OVRAPI void Finalize(D3D12CommandList *commandList) final;
		OVRAPI void PresentBackBuffer() final;

		OVRAPI ID3D12Resource *GetD3D12ColorBuffer(int i) const final;
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; }

		OVRAPI void ResizeOpenVRSwapChain(const Vector2 &size) final;
		OVRAPI Vector2 GetOpenVRSwapChainSize() const final { return GetSize(); }
		OVRAPI const Window::SwapChainDescriptor &GetOpenVRSwapChainDescriptor() const final { return _descriptor; }
		OVRAPI Framebuffer *GetOpenVRSwapChainFramebuffer() const final;

	protected:
		OpenVRD3D12SwapChain(const Window::SwapChainDescriptor &descriptor, vr::IVRSystem *system);

	private:
		bool _isFirstRender;

		RNDeclareMetaAPI(OpenVRD3D12SwapChain, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRD3D12SWAPCHAIN_H_ */
