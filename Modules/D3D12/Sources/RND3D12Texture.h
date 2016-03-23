//
//  RND3D12Texture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12TEXTURE_H_
#define __RAYNE_D3D12TEXTURE_H_

#include "RND3D12.h"

namespace RN
{
	class D3D12Renderer;
	class D3D12StateCoordinator;

	class D3D12Texture : public Texture
	{
	public:
		friend class D3D12Renderer;

		D3DAPI ~D3D12Texture() override;

		D3DAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		D3DAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		D3DAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;
		D3DAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const final;

		D3DAPI void GenerateMipMaps() final;
		D3DAPI void SetParameter(const Parameter &parameter) final;
		D3DAPI bool HasColorChannel(ColorChannel channel) const final;

		D3DAPI void *__GetUnderlyingTexture() const { return _texture; }
		D3DAPI void *__GetUnderlyingSampler() const { return _sampler; }

	private:
		D3D12Texture(D3D12Renderer *renderer, D3D12StateCoordinator *coordinator, void *texture, const Descriptor &descriptor);

		D3D12Renderer *_renderer;
		D3D12StateCoordinator *_coordinator;
		void *_texture;
		void *_sampler;

		void *_data;

		RNDeclareMetaAPI(D3D12Texture, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12TEXTURE_H_ */
