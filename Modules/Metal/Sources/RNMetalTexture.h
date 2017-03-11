//
//  RNMetalTexture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALTEXTURE_H_
#define __RAYNE_METALTEXTURE_H_

#include "RNMetal.h"

namespace RN
{
	class MetalRenderer;
	class MetalStateCoordinator;

	class MetalTexture : public Texture
	{
	public:
		friend class MetalRenderer;

		MTLAPI ~MetalTexture() override;

		MTLAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		MTLAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		MTLAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;
		MTLAPI void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const final;

		MTLAPI void GenerateMipMaps() final;
		MTLAPI void SetParameter(const Parameter &parameter) final;

		MTLAPI bool HasColorChannel(ColorChannel channel) const final;

		static MTLPixelFormat PixelFormatForTextureFormat(Format format);

		void *__GetUnderlyingTexture() const { return _texture; }
		void *__GetUnderlyingSampler() const { return _sampler; }

	private:
		MetalTexture(MetalRenderer *renderer, MetalStateCoordinator *coordinator, void *texture, const Descriptor &descriptor);

		MetalRenderer *_renderer;
		MetalStateCoordinator *_coordinator;
		void *_texture;
		void *_sampler;

		RNDeclareMetaAPI(MetalTexture, MTLAPI)
	};
}


#endif /* __RAYNE_METALTEXTURE_H_ */
