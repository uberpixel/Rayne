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

		static MTLPixelFormat PixelFormatForTextureFormat(Format format);
		static MTLTextureDescriptor *DescriptorForTextureDescriptor(const Descriptor &descriptor, bool isIOSurfaceBacked = false);

		void *__GetUnderlyingTexture() const { return _texture; }

	private:
		MetalTexture(MetalRenderer *renderer, void *texture, const Descriptor &descriptor);

		MetalRenderer *_renderer;
		void *_texture;

		RNDeclareMetaAPI(MetalTexture, MTLAPI)
	};
}


#endif /* __RAYNE_METALTEXTURE_H_ */
