//
//  RNTextureLoader.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TEXTURELOADER_H__
#define __RAYNE_TEXTURELOADER_H__

#include "RNBase.h"
#include "RNFile.h"
#include "RNObject.h"
#include "RNTexture.h"

namespace RN
{
	class TextureLoader
	{
	public:
		TextureLoader(const std::string& name);
		~TextureLoader();
		
		uint32 Width() const { return _width; }
		uint32 Height() const { return _height; }
		
		const void *Data() const { return _data; }
		Texture::Format Format() const { return _format; }
		
	private:
		bool LoadPNGTexture(FILE *file);
		
		uint32 _width;
		uint32 _height;
		
		void *_data;
		Texture::Format _format;
	};
}

#endif /* __RAYNE_TEXTURELOADER_H__ */
