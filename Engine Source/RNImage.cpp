//
//  RNImage.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNImage.h"

namespace RN
{
	RNDeclareMeta(Image)
	
	Image::Image(class Texture *texture)
	{
		RN_ASSERT0(texture);
		_texture = texture->Retain();
	}
	
	Image::Image(const std::string& file)
	{
		TextureParameter parameter;
		parameter.mipMaps = 0;
		parameter.generateMipMaps = false;
		
		_texture = new RN::Texture(file, parameter, true);
	}
	
	Image::~Image()
	{
		_texture->Release();
	}
	
	
	
	Image *Image::WithFile(const std::string& file)
	{
		Image *image = new Image(file);
		return image->Autorelease();
	}
}
