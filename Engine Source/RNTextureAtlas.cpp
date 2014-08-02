//
//  RNTextureAtlas.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTextureAtlas.h"
#include "RNAlgorithm.h"

namespace RN
{
	TextureAtlas::TextureAtlas(uint32 width, uint32 height, const Texture::Parameter &parameter) :
		Texture2D(parameter)
	{
		Initialize(width, height);
	}
	
	TextureAtlas::TextureAtlas(uint32 width, uint32 height, bool isLinear, const Texture::Parameter &parameter) :
		Texture2D(parameter, isLinear)
	{
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
		_width  = _maxWidth = width;
		_height = _maxHeight = height;
		_tag = 0;
		_mutations = 0;
		
		SetSize(_width, _height);
	}
	
	
	void TextureAtlas::SetRegionData(const Rect &region, void *data, Texture::Format format)
	{
		PixelData pdata;
		pdata.data = data;
		pdata.format = format;
		pdata.width  = region.width;
		pdata.height = region.height;
		pdata.alignment = 1;
		
		UpdateRegion(pdata, region);
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
		
		PixelData pdata;
		pdata.data = data;
		pdata.format = Format::RGBA8888;
		pdata.width  = _width;
		pdata.height = _height;
		pdata.scaleFactor = GetScaleFactor();
		
		memset(nData, 0, nWidth * nHeight * 4);
		GetData(pdata);
		
		for(uint32 y = 0; y < _height; y ++)
		{
			uint8 *row = data + (y * _width * 4);
			uint8 *nrow = nData + (y * nWidth * 4);
			
			std::copy(row, row + (_width * 4), nrow);
		}
		
		pdata.data = nData;
		pdata.width = nWidth;
		pdata.height = nHeight;
		
		SetData(pdata);
		
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
		size_t smallestDimenion = 0;
		size_t bestIndex = kRNNotFound;
		
		for(size_t i = 0; i < _regions.size(); i ++)
		{
			TextureRegion &region = _regions[i];
			
			if(region.isFree && (region.rect.width >= width && region.rect.height >= height))
			{
				size_t dimension = region.rect.width * region.rect.height;
				
				if(dimension < smallestDimenion || smallestDimenion == 0)
				{
					smallestDimenion = dimension;
					bestIndex = i;
				}
			}
		}
		
		if(bestIndex == kRNNotFound)
			throw Exception(Exception::Type::RangeException, "Not enough free space found in atlas texture!");
		
		
		TextureRegion &source = _regions[bestIndex];
		
		if(source.rect.width == width && source.rect.height == height)
		{
			source.isFree = false;
			return source.rect;
		}
		
		
		TextureRegion occupator;
		occupator.rect   = Rect(source.rect.x, source.rect.y, width, height);
		occupator.isFree = false;
		
		
		std::vector<TextureRegion> newRegions;
		newRegions.push_back(occupator);
		
		if(source.rect.width > width)
		{
			TextureRegion temp;
			temp.rect = Rect(source.rect.x + width, source.rect.y, source.rect.width - width, height);
			temp.isFree = true;
			
			newRegions.push_back(temp);
		}
		
		source.rect.height -= height;
		source.rect.y += height;
		
		if(source.rect.height <= 0.0f)
		{
			auto iterator = _regions.begin() + bestIndex;
			_regions.erase(iterator);
		}
		
		_regions.insert(_regions.end(), newRegions.begin(), newRegions.end());
		_mutations ++;
		
		return occupator.rect;
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
