//
//  RNFont.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include "RNFont.h"

const char *kRNCommonCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890.,-;:_-+*/!\"§$%&()=?<>' ";

namespace RN
{
	RNDeclareMeta(Font)

	struct FontInternals
	{
		FT_Library library;
		FT_Face face;
	};
	
	
#define _internals (reinterpret_cast<FontInternals *>(_finternals))
	
	Font::Font(const std::string& name, float size)
	{
		TextureParameter parameter;
		
		parameter.mipMaps = 0;
		parameter.generateMipMaps = false;
		parameter.format = TextureParameter::Format::RGBA8888;
		parameter.wrapMode = TextureParameter::WrapMode::Clamp;
		parameter.filter = TextureParameter::Filter::Nearest;
		
		_texture = new TextureAtlas(1024, 1024, parameter);
		
		_finternals = 0;
		
		_size = size;
		_fontPath = name;
		
		_filtering = false;
		_filterWeights[0] = 0x10;
		_filterWeights[1] = 0x40;
		_filterWeights[2] = 0x70;
		_filterWeights[3] = 0x40;
		_filterWeights[4] = 0x10;
		
		RenderCharactersFromString(String(kRNCommonCharacters));
	}
	
	Font::~Font()
	{
		_texture->Release();
		DropInternals();
	}
	
	
	void Font::InitializeInternals()
	{
		if(_finternals)
			return;
		
		_finternals = new FontInternals;
		
		//FT_Error error;
		FT_Init_FreeType(&_internals->library);
		
		FT_UInt hres = 64;
		FT_Matrix matrix;
		
		matrix.xx = (FT_Fixed)((1.0f / hres) * 0x10000L);
		matrix.xy = 0.0f;
		matrix.yx = 0.0f;
		matrix.yy = (FT_Fixed)(1.0f * 0x10000L);
		
		FT_New_Face(_internals->library, _fontPath.c_str(), 0, &_internals->face);
		
		FT_Select_Charmap(_internals->face, FT_ENCODING_UNICODE);
		FT_Set_Char_Size(_internals->face, (int)(_size * 64.0f), 0, 72 * hres, 72);
		
		FT_Set_Transform(_internals->face, &matrix, 0);
	}
	
	void Font::DropInternals()
	{
		if(!_finternals)
			return;
		
		FT_Done_Face(_internals->face);
		FT_Done_FreeType(_internals->library);
		
		delete _internals;
		
		_finternals = 0;
	}
	
	
	void Font::RenderGlyph(UniChar character)
	{
		FT_Int32 flags = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT;
		FT_Error error;
		
		FT_UInt glyphIndex = FT_Get_Char_Index(_internals->face, character);
		
		if(_filtering)
		{
			FT_Library_SetLcdFilter(_internals->library, FT_LCD_FILTER_LIGHT);
			FT_Library_SetLcdFilterWeights(_internals->library, _filterWeights);
			
			flags |= FT_LOAD_TARGET_LCD;
		}
		
		error = FT_Load_Glyph(_internals->face, glyphIndex, flags);
		
		
		FT_GlyphSlot slot = _internals->face->glyph;
		FT_Bitmap bitmap  = slot->bitmap;
		
		size_t size = bitmap.width * bitmap.rows;
		uint32 width  = bitmap.width;
		uint32 height = bitmap.rows;

		uint8 *data = new uint8[size * 4];
		uint8 *temp = data;
		
		switch(bitmap.pixel_mode)
		{
			case FT_PIXEL_MODE_MONO:
				for(size_t i=0; i<size; i++)
				{
					uint8 pixel = (bitmap.buffer[i] > 0) ? 255 : 0;
					
					*temp ++ = pixel;
					*temp ++ = pixel;
					*temp ++ = pixel;
					*temp ++ = pixel;
				}
				break;
				
			case FT_PIXEL_MODE_GRAY:
				for(size_t i=0; i<size; i++)
				{
					*temp ++ = bitmap.buffer[i];
					*temp ++ = bitmap.buffer[i];
					*temp ++ = bitmap.buffer[i];
					*temp ++ = bitmap.buffer[i];
				}
				break;
				
			case FT_PIXEL_MODE_LCD:
			{
				width /= 3;
				
				for(size_t i=0; i<size; i++)
				{
					*temp ++ = bitmap.buffer[(i * 3) + 0];
					*temp ++ = bitmap.buffer[(i * 3) + 1];
					*temp ++ = bitmap.buffer[(i * 3) + 2];
					
					*temp ++ = 255;
				}
				
				break;
			}
				
			default:
				return;
		}
		
		Rect rect = _texture->AllocateRegion(width + 1, height + 1);
		rect.width  -= 1.0f;
		rect.height -= 1.0f;
		
		_texture->SetRegionData(rect, data, TextureParameter::Format::RGBA8888);
		delete [] data;
		
		Glyph glyph;
		glyph._character = character;
		glyph._region    = rect;
		
		glyph._offset_x = slot->bitmap_top;
		glyph._offset_y = slot->bitmap_left;
		
		glyph._u0 = rect.x / _texture->Width();
		glyph._v0 = rect.y / _texture->Height();
		glyph._u1 = (rect.x + rect.width) / _texture->Width();
		glyph._v1 = (rect.y + rect.height) / _texture->Height();
		
		FT_Load_Glyph(_internals->face, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
		slot = _internals->face->glyph;
		
		glyph._advance_x = slot->advance.x / 64.0f;
		glyph._advance_y = slot->advance.y / 64.0f;
		
		_glyphs.insert(std::unordered_map<wchar_t, Glyph>::value_type(glyph._character, glyph));
	}
	
	void Font::RenderCharactersFromString(const String& string)
	{
		InitializeInternals();
		
		for(uint32 i=0; i<string.Length(); i++)
		{
			UniChar character = string.CharacterAtIndex(i);
			if(_glyphs.find(character) != _glyphs.end())
				continue;
			
			RenderGlyph(character);
		}
		
		DropInternals();
	}
	
#undef _internals
}

