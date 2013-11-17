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
		TextureAtlas(uint32 width, uint32 height, const Texture::Parameter& parameter);
		TextureAtlas(uint32 width, uint32 height, bool linear, const Texture::Parameter& parameter);
		~TextureAtlas() override;
		
		Rect AllocateRegion(uint32 width, uint32 height);
		void SetRegionData(const Rect& region, void *data, Texture::Parameter::Format format);
		
		void SetMaxSize(uint32 maxWidth, uint32 maxHeight);
		uint32 GetTag() const { return _tag; }
	
	private:
		void Initialize(uint32 width, uint32 height);
		
		Rect TryAllocateRegion(uint32 width, uint32 height);
		void IncreaseSize();
		bool CanIncreaseSize();
		
		struct TextureRegion
		{
			Rect rect;
			bool isFree;
		};
		
		uint32 _tag;
		uint32 _mutations;
		uint32 _width, _height;
		uint32 _maxWidth, _maxHeight;
		std::vector<TextureRegion> _regions;
	};
}

#endif /* __RAYNE_TEXTUREATLAS_H__ */
