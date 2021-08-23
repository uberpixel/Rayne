//
//  RNOpenXRD3D12SwapChain.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_OPENXRD3D12SWAPCHAIN_H_
#define __RAYNE_OPENXRD3D12SWAPCHAIN_H_

#include "RND3D12SwapChain.h"
#include "RNOpenXRSwapChain.h"

namespace RN
{
	class OpenXRD3D12SwapChain : public D3D12SwapChain, OpenXRSwapChain
	{
	public:
		friend class OpenXRWindow;

		OXRAPI ~OpenXRD3D12SwapChain();

		OXRAPI void AcquireBackBuffer() final;
		OXRAPI void Prepare(D3D12CommandList *commandList) final;
		OXRAPI void Finalize(D3D12CommandList *commandList) final;
		OXRAPI void PresentBackBuffer() final;

		OXRAPI ID3D12Resource *GetD3D12ColorBuffer(int i) const final;
		const Window::SwapChainDescriptor &GetDescriptor() const { return _descriptor; }

		OXRAPI void ResizeSwapChain(const Vector2 &size) final;
		Vector2 GetSwapChainSize() const final { return GetSize(); }
		const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final { return _descriptor; }
		OXRAPI Framebuffer *GetSwapChainFramebuffer() const final;

	protected:
		OpenXRD3D12SwapChain(const OpenXRWindow *window, const Window::SwapChainDescriptor &descriptor, const Vector2 &size);

	private:
		ID3D12Resource **_swapchainImages;

		RNDeclareMetaAPI(OpenXRD3D12SwapChain, OXRAPI)
	};
}


#endif /* __RAYNE_OPENXRD3D12SWAPCHAIN_H_ */
