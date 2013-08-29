//
//  RNTextureAtlas.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTextureAtlas.h"
#include "RNAlgorithm.h"

namespace RN
{
	TextureAtlas::TextureAtlas(uint32 width, uint32 height, const TextureParameter& parameter) :
		Texture(parameter)
	{
		SetData(0, width, height, parameter.format);
		Initialize(width, height);
	}
	
	TextureAtlas::TextureAtlas(uint32 width, uint32 height, bool isLinear, const TextureParameter& parameter) :
		Texture(parameter, isLinear)
	{
		SetData(0, width, height, parameter.format);
		Initialize(width, height);
	}
	
	TextureAtlas::~TextureAtlas()
	{}
	
	void TextureAtlas::Initialize(uint32 width, uint32 height)
	{
		TextureRegion region;
		region.rect = Rect(0.0f, 0.0f, width, height);
		region.isFree = true;
		
		_regions.push_back(region);
		_width = _maxWidth = width;
		_height = _maxHeight = height;
		_tag = 0;
	}
	
	
	void TextureAtlas::SetRegionData(const Rect& region, void *data, TextureParameter::Format format)
	{
		UpdateRegion(data, region, format);
	}
	
	void TextureAtlas::SetMaxSize(uint32 maxWidth, uint32 maxHeight)
	{
		RN_ASSERT(maxWidth >= _width && maxHeight >= _height, "The maximum size must be greater than the current size");
		
		_maxWidth  = maxWidth;
		_maxHeight = maxHeight;
	}
	
	
	void TextureAtlas::IncreaseSize()
	{
		uint32 nWidth = _width;
		uint32 nHeight = _height;
		
		if(_width < _maxWidth)
			nWidth = NextPowerOfTwo(_width + 1);
		
		if(_height < _maxHeight)
			nHeight = NextPowerOfTwo(_height + 1);
		
		// Insert free regions
		if(nWidth > _width)
		{
			TextureRegion region;
			region.rect = Rect(_width, 0, nWidth - _width, _height);
			region.isFree = true;
			
			_regions.push_back(region);
		}
		
		if(nHeight > _height)
		{
			TextureRegion region;
			region.rect = Rect(0, _height, _width, nHeight - _height);
			region.isFree = true;
			
			_regions.push_back(region);
		}
		
		if(nWidth > _width && nHeight > _height)
		{
			TextureRegion region;
			region.rect = Rect(_width, _height, nWidth - _width, nHeight - _height);
			region.isFree = true;
			
			_regions.push_back(region);
		}
		
		// Update the data
		uint8 *data = new uint8[_width * _height * 4];
		uint8 *nData = new uint8[nWidth * nHeight * 4];
		
		memset(nData, 0, nWidth * nHeight * 4);
		GetData(data, TextureParameter::Format::RGBA8888);
		
		for(uint32 y = 0; y < _height; y ++)
		{
			uint8 *row = data + (y * _width * 4);
			uint8 *nrow = nData + (y * nWidth * 4);
			
			std::copy(row, row + (_width * 4), nrow);
		}
		
		SetData(nData, nWidth, nHeight, TextureParameter::Format::RGBA8888);
		
		delete [] nData;
		delete [] data;
		
		_width = nWidth;
		_height = nHeight;
		_tag ++;
	}
	
	bool TextureAtlas::CanIncreaseSize()
	{
		return (_width < _maxWidth || _height < _maxHeight);
	}
	
	Rect TextureAtlas::TryAllocateRegion(uint32 width, uint32 height)
	{
		size_t biggestDimension = 0;
		size_t biggestIndex = kRNNotFound;
		
		for(size_t i=0; i<_regions.size(); i++)
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
		
		if(biggestIndex != kRNNotFound)
		{
			TextureRegion& source = _regions[(int)biggestIndex];
			
			if(source.rect.width == width && source.rect.height == height)
			{
				source.isFree = false;
				return source.rect;
			}
			
			
			
			TextureRegion occupator;
			Rect rect = Rect(source.rect.x, source.rect.y, width, height);
			
			occupator.rect = rect;
			occupator.isFree = false;
			
			if(source.rect.width == width)
			{
				source.rect.height -= height;
				source.rect.y += height;
				
				_regions.push_back(occupator);
				return rect;
			}
			
			if(source.rect.height == height)
			{
				source.rect.width -= width;
				source.rect.x += width;
				
				_regions.push_back(occupator);
				return rect;
			}
			
			
			TextureRegion next;
			next.rect.x = occupator.rect.x + occupator.rect.width;
			next.rect.y = occupator.rect.y;
			next.rect.width = source.rect.width - width;
			next.rect.height = height;
			
			next.isFree = true;
			
			source.rect.height -= height;
			source.rect.y += height;
			
			_regions.push_back(occupator);
			_regions.push_back(next);
			
			return rect;
		}
		
		throw Exception(Exception::Type::RangeException, "Not enough free space found in atlas texture!");
	}
	
	Rect TextureAtlas::AllocateRegion(uint32 width, uint32 height)
	{
		while(1)
		{
			try
			{
				Rect rect = TryAllocateRegion(width, height);
				return rect;
			}
			catch(Exception e)
			{
				if(CanIncreaseSize())
				{
					IncreaseSize();
					continue;
				}
				
				throw e;
			}
		}
	}
}
