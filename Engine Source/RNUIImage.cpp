//
//  RNUIImage.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImage.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Image)
		
		Image::Image(class Texture *texture) :
			_atlas(UI::Atlas(Vector2(0.0f), Vector2(1.0f)))
		{
			RN_ASSERT0(texture);
			_texture = texture->Retain();
		}
		
		Image::Image(const std::string& file) :
			_atlas(UI::Atlas(0.0f, 0.0f, 1.0f, 1.0f))
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
		
		
		void Image::SetAtlas(const struct Atlas& atlas, bool normalized)
		{
			_atlas = atlas;
			
			if(!normalized)
			{
				uint32 width = Width(false);
				uint32 height = Height(false);
				
				_atlas.u1 /= width;
				_atlas.u2 = (width - _atlas.u2) / width;
				
				_atlas.v1 /= height;
				_atlas.v2 = (height - _atlas.v2) / height;
			}
		}
		
		void Image::SetEdgeInsets(const EdgeInsets& insets)
		{
			_insets = insets;
		}
		
		uint32 Image::Width(bool atlasApplied) const
		{
			uint32 width = _texture->Width();
			if(atlasApplied)
			{
				float nwidth  = _atlas.u2 - _atlas.u1;
				width = nwidth * width;
			}
			
			return width;
		}
		
		uint32 Image::Height(bool atlasApplied) const
		{
			uint32 height = _texture->Height();
			if(atlasApplied)
			{
				float nheight = _atlas.v2 - _atlas.v1;
				height = nheight * height;
			}
			
			return height;
		}
	}
}
