//
//  RNTextureAtlas.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTextureAtlas.h"

namespace RN
{
	TextureAtlas::TextureAtlas(uint32 width, uint32 height, const TextureParameter& parameter) :
		Texture(parameter)
	{
		SetData(0, width, height, parameter.format);
		
		TextureRegion region;
		
		region.rect = Rect(0.0f, 0.0f, width, height);
		region.rect.Inset(1.0f, 1.0f);
		region.rect.Integral();
		
		region.isFree = true;
		
		_regions.AddObject(region);
		_data = static_cast<uint8 *>(calloc(width * height * 4, 1));
	}
	
	TextureAtlas::~TextureAtlas()
	{
		if(_data)
			free(_data);
	}
	
	
	void TextureAtlas::Bind()
	{
		/*if(_data)
		{
			SetData(_data, Width(), Height(), Texture::FormatRGBA8888);
			free(_data);
			
			_data = 0;
		}*/
		
		Texture::Bind();
	}
	
	Rect TextureAtlas::AllocateRegion(uint32 width, uint32 height)
	{
		size_t biggestDimension = 0;
		machine_uint biggestIndex = RN_NOT_FOUND;
		
		for(machine_uint i=0; i<_regions.Count(); i++)
		{
			TextureRegion& region = _regions[(int)i];
			
			if(region.isFree && (region.rect.width >= width && region.rect.height >= height))
			{
				size_t dimension = region.rect.width * region.rect.height;
				
				if(dimension > biggestDimension)
				{
					biggestDimension = dimension;
					biggestIndex = i;
				}
			}
		}
		
		if(biggestIndex != RN_NOT_FOUND)
		{
			TextureRegion& source = _regions[(int)biggestIndex];
			
			if(source.rect.width == width && source.rect.height == height)
			{
				source.isFree = false;
				return source.rect;
			}
			
			TextureRegion occupator;
			occupator.rect = Rect(source.rect.x, source.rect.y, width, height);
			occupator.isFree = false;
			
			_regions.AddObject(occupator);
			
			if(source.rect.width == width)
			{
				source.rect.height -= height;
				source.rect.y += height;
				
				return occupator.rect;
			}
			
			if(source.rect.height == height)
			{
				source.rect.width -= width;
				source.rect.x += width;
				
				return occupator.rect;
			}
			
			
			
			TextureRegion next;
			next.rect.x = occupator.rect.x + occupator.rect.width;
			next.rect.y = occupator.rect.y;
			next.rect.width = source.rect.width - width;
			next.rect.height = height;
			
			next.isFree = true;
			
			source.rect.height -= height;
			source.rect.y += height;
			
			_regions.AddObject(next);
			return occupator.rect;
		}
		
		throw "Shit...";
	}
	
	void TextureAtlas::SetRegionData(const Rect& region, void *data, TextureParameter::Format format)
	{
		/*if(!_data)
		{
			_data = static_cast<uint8 *>(calloc(Width() * Height() * 4, 1));
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, _data);
		}*/
		
		UpdateRegion(data, region, format);
	}
}
