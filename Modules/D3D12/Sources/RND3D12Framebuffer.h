//
//  RND3D12Framebuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12FRAMEBUFFER_H__
#define __RAYNE_D3D12FRAMEBUFFER_H__

#include "RND3D12.h"
#include "RND3D12SwapChain.h"
#include "RND3D12Texture.h"

namespace RN
{
	class D3D12Framebuffer : public Framebuffer
	{
	public:
		friend class D3D12Renderer;
		friend class D3D12StateCoordinator;

		struct D3D12ColorTargetView
		{
			TargetView targetView;
			D3D12_RENDER_TARGET_VIEW_DESC d3dTargetViewDesc;
		};

		struct D3D12DepthStencilTargetView
		{
			TargetView targetView;
			D3D12_DEPTH_STENCIL_VIEW_DESC d3dTargetViewDesc;
		};

		D3DAPI D3D12Framebuffer(const Vector2 &size, D3D12SwapChain *swapChain, D3D12Renderer *renderer, Texture::Format colorFormat, Texture::Format depthStencilFormat);
		D3DAPI D3D12Framebuffer(const Vector2 &size, D3D12Renderer *renderer);

		D3DAPI ~D3D12Framebuffer();

		D3DAPI void SetColorTarget(const TargetView &target, uint32 index = 0) final;
		D3DAPI void SetDepthStencilTarget(const TargetView &target) final;

		D3DAPI Texture *GetColorTexture(uint32 index = 0) const final;
		D3DAPI Texture *GetDepthStencilTexture() const final;
		D3DAPI uint8 GetSampleCount() const final;

		D3D12SwapChain *GetSwapChain() const { return _swapChain; }
		D3DAPI ID3D12Resource *GetSwapChainColorBuffer() const;
		D3DAPI ID3D12Resource *GetSwapChainDepthBuffer() const;

		D3DAPI void WillUpdateSwapChain();
		D3DAPI void DidUpdateSwapChain(Vector2 size, Texture::Format colorFormat, Texture::Format depthStencilFormat);

	private:
		void PrepareAsRendertargetForFrame(uint32 frame);
		void SetAsRendertarget(D3D12CommandList *commandList) const;
		void ClearColorTargets(D3D12CommandList *commandList, const Color &color);
		void ClearDepthStencilTarget(D3D12CommandList *commandList, float depth, uint8 stencil);

		CD3DX12_CPU_DESCRIPTOR_HANDLE *_rtvHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE *_dsvHandle;

		uint8 _sampleCount;

		D3D12Renderer *_renderer;
		uint32 _frameLastUsed;

		ID3D12Resource **_swapChainColorBuffers;
		ID3D12Resource **_swapChainDepthBuffers;
		WeakRef<D3D12SwapChain> _swapChain;

		std::vector<D3D12ColorTargetView *> _colorTargets;
		D3D12DepthStencilTargetView *_depthStencilTarget;

		RNDeclareMetaAPI(D3D12Framebuffer, D3DAPI)
	};
}

#endif //__RAYNE_D3D12FRAMEBUFFER_H__
