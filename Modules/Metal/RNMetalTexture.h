//
//  RNMetalTexture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALTEXTURE_H_
#define __RAYNE_METALTEXTURE_H_

#include <Rayne.h>

namespace RN
{
	class MetalRenderer;
	class MetalStateCoordinator;

	class MetalTexture : public Texture
	{
	public:
		friend class MetalRenderer;

		RNAPI ~MetalTexture() override;

		RNAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		RNAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		RNAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;

		RNAPI void GenerateMipMaps() final;
		RNAPI void SetParameter(const Parameter &parameter) final;

		RNAPI void *__GetUnderlyingTexture() const { return _texture; }
		RNAPI void *__GetUnderlyingSampler() const { return _sampler; }

	private:
		MetalTexture(MetalRenderer *renderer, MetalStateCoordinator *coordinator, void *texture, const Descriptor &descriptor);

		MetalRenderer *_renderer;
		MetalStateCoordinator *_coordinator;
		void *_texture;
		void *_sampler;

		RNDeclareMeta(MetalTexture)
	};
}


#endif /* __RAYNE_METALTEXTURE_H_ */
