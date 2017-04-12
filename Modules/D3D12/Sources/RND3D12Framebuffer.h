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

namespace RN
{
	class D3D12Framebuffer : public Framebuffer
	{
	public:
		friend class D3D12Renderer;
		friend class D3D12StateCoordinator;

		D3DAPI D3D12Framebuffer(const Vector2 &size, const Descriptor &descriptor, D3D12SwapChain *swapChain, D3D12Renderer *renderer);
		D3DAPI ~D3D12Framebuffer();

		D3DAPI Texture *GetColorTexture() const final;
		D3DAPI Texture *GetDepthTexture() const final;
		D3DAPI Texture *GetStencilTexture() const final;

		ID3D12Resource *GetRenderTarget() const;
		ID3D12Resource *GetDepthBuffer() const { return _depthStencilBuffer; }
		D3D12SwapChain *GetSwapChain() const { return _swapChain; }

	private:
		D3D12Renderer *_renderer;
		WeakRef<D3D12SwapChain> _swapChain;
		ID3D12Resource **_renderTargets;
		ID3D12Resource *_depthStencilBuffer;

		DXGI_FORMAT _colorFormat;

		RNDeclareMetaAPI(D3D12Framebuffer, D3DAPI)
	};
}

#endif //__RAYNE_D3D12FRAMEBUFFER_H__
