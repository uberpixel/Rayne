//
//  RNTextureAtlas.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TEXTUREATLAS_H__
#define __RAYNE_TEXTUREATLAS_H__

#include "RNBase.h"
#include "RNTexture.h"
#include "RNRect.h"
#include "RNArray.h"
#include "RNVector.h"

namespace RN
{
	class TextureAtlas : public Texture
	{
	public:
		TextureAtlas(uint32 width, uint32 height, const TextureParameter& parameter);
		virtual ~TextureAtlas();
		
		Rect AllocateRegion(uint32 width, uint32 height);
		void SetRegionData(const Rect& region, void *data, TextureParameter::Format format);
		
		virtual void Bind();
	
	private:
		struct TextureRegion
		{
			Rect rect;
			bool isFree;
		};
		
		std::vector<TextureRegion> _regions;
		uint8 *_data;
	};
}

#endif /* __RAYNE_TEXTUREATLAS_H__ */
