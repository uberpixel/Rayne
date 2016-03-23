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

namespace RN
{
	class D3D12Renderer;
	class D3D12Framebuffer : public Framebuffer
	{
	public:
		D3DAPI D3D12Framebuffer(const Vector2 &size, const Descriptor &descriptor, IDXGISwapChain3 *swapChain, D3D12Renderer *renderer);
		D3DAPI ~D3D12Framebuffer();

		D3DAPI Texture *GetColorTexture() const final;
		D3DAPI Texture *GetDepthTexture() const final;
		D3DAPI Texture *GetStencilTexture() const final;

		ID3D12Resource *GetRenderTarget(size_t index) const { return _renderTargets[index]; }

	private:
		D3D12Renderer *_renderer;
		ID3D12Resource *_renderTargets[3];

		RNDeclareMetaAPI(D3D12Framebuffer, D3DAPI)
	};
}

#endif //__RAYNE_D3D12FRAMEBUFFER_H__
