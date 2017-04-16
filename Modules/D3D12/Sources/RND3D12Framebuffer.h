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

		D3DAPI D3D12Framebuffer(const Vector2 &size, const Descriptor &descriptor, D3D12SwapChain *swapChain, D3D12Renderer *renderer);
		D3DAPI ~D3D12Framebuffer();

		D3DAPI void SetColorTexture(Texture *texture) final;
		D3DAPI void SetDepthTexture(Texture *texture) final;
		D3DAPI void SetStencilTexture(Texture *texture) final;

		D3DAPI Texture *GetColorTexture() const final;
		D3DAPI Texture *GetDepthTexture() const final;
		D3DAPI Texture *GetStencilTexture() const final;

		ID3D12Resource *GetColorBuffer() const;
		ID3D12Resource *GetDepthBuffer() const;
		D3D12SwapChain *GetSwapChain() const { return _swapChain; }

	private:
		D3D12Renderer *_renderer;

		ID3D12Resource **_swapChainColorBuffers;
		WeakRef<D3D12SwapChain> _swapChain;

		D3D12Texture *_colorTexture;
		D3D12Texture *_depthTexture;
		D3D12Texture *_stencilTexture;

		DXGI_FORMAT _colorFormat;
		DXGI_FORMAT _depthFormat;

		D3D12_RTV_DIMENSION _colorDimension;
		D3D12_DSV_DIMENSION _depthDimension;

		RNDeclareMetaAPI(D3D12Framebuffer, D3DAPI)
	};
}

#endif //__RAYNE_D3D12FRAMEBUFFER_H__
