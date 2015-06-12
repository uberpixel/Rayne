//
//  RNMetalTexture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALTEXTURE_H_
#define __RAYNE_METALTEXTURE_H_

#include "../../Base/RNBase.h"
#include "../RNTexture.h"

namespace RN
{
	class MetalRenderer;

	class MetalTexture : public Texture
	{
	public:
		friend class MetalRenderer;

		RNAPI ~MetalTexture() override;

		RNAPI void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		RNAPI void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) final;
		RNAPI void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) final;

		RNAPI void SetGenerateMipMaps() final;

	private:
		MetalTexture(void *texture, const Descriptor &descriptor);

		void *_texture;

		RNDeclareMeta(MetalTexture)
	};
}


#endif /* __RAYNE_METALTEXTURE_H_ */
