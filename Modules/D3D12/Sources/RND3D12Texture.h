//
//  RND3D12Texture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12TEXTURE_H_
#define __RAYNE_D3D12TEXTURE_H_

#include <Rayne.h>

namespace RN
{
	class D3D12Renderer;
	class D3D12StateCoordinator;

	class D3D12Texture : public Texture
	{
	public:
		friend class D3D12Renderer;

		RNAPI ~D3D12Texture() override;

		RNAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		RNAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		RNAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;

		RNAPI void GenerateMipMaps() final;
		RNAPI void SetParameter(const Parameter &parameter) final;

		RNAPI void *__GetUnderlyingTexture() const { return _texture; }
		RNAPI void *__GetUnderlyingSampler() const { return _sampler; }

	private:
		D3D12Texture(D3D12Renderer *renderer, D3D12StateCoordinator *coordinator, void *texture, const Descriptor &descriptor);

		D3D12Renderer *_renderer;
		D3D12StateCoordinator *_coordinator;
		void *_texture;
		void *_sampler;

		RNDeclareMeta(D3D12Texture)
	};
}


#endif /* __RAYNE_D3D12TEXTURE_H_ */
