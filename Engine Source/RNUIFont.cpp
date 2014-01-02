//
//  RNUIFont.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include "RNUIFont.h"
#include "RNPathManager.h"
#include "RNBaseInternal.h"
#include "RNFileManager.h"
#include "RNKernel.h"

const char *kRNCommonCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890.,-;:_-+*/!\"§$%&()=?<>' ";

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Font)
		
		float Glyph::GetKerning(UniChar character) const
		{
			auto iterator = _kerning.find(character);
			return (iterator != _kerning.end()) ? iterator->second : 0.0f;
		}
		

		struct FontInternals
		{
			FT_Library library;
			FT_Face face;
		};
		
		
		Font::Font(const std::string& name, float size) :
			Font(name, size, FontDescriptor())
		{}
		
		Font::Font(const std::string& name, float size, const FontDescriptor& descriptor) :
			_descriptor(descriptor)
		{
			Texture::Parameter parameter;
			
			parameter.maxMipMaps = descriptor.mipMaps ? 1000 : 0;
			parameter.format   = GetFiltering() ? Texture::Format::RGB888 : Texture::Format::R8;
			parameter.wrapMode = Texture::WrapMode::Clamp;
			parameter.filter   = descriptor.textureFilter ? Texture::Filter::Linear : Texture::Filter::Nearest;
			
			_scale = Kernel::GetSharedInstance()->GetScaleFactor();
			_texture = new TextureAtlas(128 * _scale, 128 * _scale, true, parameter);
			_texture->SetMaxSize(4096 * _scale, 4096 * _scale);
			
			_textureTag = _texture->GetTag();
			
			_size       = size;
			_faceIndex  = 0;
			
			_filterWeights[0] = 0x10;
			_filterWeights[1] = 0x40;
			_filterWeights[2] = 0x70;
			_filterWeights[3] = 0x40;
			_filterWeights[4] = 0x10;
			
			ResolveFontName(name);
			Initialize();
		}
		
		
		Font::~Font()
		{
			_texture->Release();
			DropInternals();
		}
		
		Font *Font::WithName(const std::string& name, float size)
		{
			Font *font = new Font(name, size);
			return font->Autorelease();
		}
		
		Font *Font::WithNameAndDescriptor(const std::string& name, float size, const FontDescriptor& descriptor)
		{
			Font *font = new Font(name, size, descriptor);
			return font->Autorelease();
		}
		
		
		
		void Font::Initialize()
		{
			InitializeInternals();
			
			_unitsPerEM = _internals->face->units_per_EM;
			
			_height  = ConvertFontUnit(_internals->face->height);
			_ascent  = ConvertFontUnit(_internals->face->ascender);
			_descent = ConvertFontUnit(_internals->face->descender);
			_leading = _height - _ascent + _descent;
			
			for(size_t i=0; i<strlen(kRNCommonCharacters); i++)
			{
				RenderGlyph(CodePoint::ConvertCharacter(kRNCommonCharacters[i]));
			}
			
			RenderGlyph(CodePoint::Ellipsis());
			UpdateAtlas();
			DropInternals();
		}
		
		void Font::ResolveFontName(const std::string& name)
		{
			std::string path;
			
			try
			{
				path = FileManager::GetSharedInstance()->GetFilePathWithName(name);
			}
			catch(Exception e)
			{
#if RN_PLATFORM_MAC_OS
				@autoreleasepool
				{
					CTFontSymbolicTraits traits = 0;
					traits |= (_descriptor.style & FontDescriptor::FontStyleBold) ? kCTFontTraitBold : 0;
					traits |= (_descriptor.style & FontDescriptor::FontStyleItalic) ? kCTFontTraitItalic : 0;
					
					CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingASCII);
					CTFontRef source = CTFontCreateWithName(fontName, _size, nullptr);
					
					if(!source)
					{
						CFRelease(fontName);
						throw Exception(Exception::Type::GenericException, "No font with name " + name);
					}
					
					CTFontRef font = CTFontCreateCopyWithSymbolicTraits(source, 0.0f, nullptr, traits, kCTFontTraitBold | kCTFontTraitItalic);
					
					if(!font)
						font = static_cast<CTFontRef>(CFRetain(source));
					
					NSURL *url = reinterpret_cast<NSURL *>(const_cast<void *>(CTFontCopyAttribute(font, kCTFontURLAttribute)));
					path = [[url path] UTF8String];

					[url release];
					
					CFRelease(font);
					CFRelease(source);
					CFRelease(fontName);
				}
#endif
				
#if RN_PLATFORM_WINDOWS
				HKEY hKey;
				LONG result;
				
				std::wstring faceName(name.begin(), name.end());
				
				result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey);
				if(result != ERROR_SUCCESS)
					throw e;
				
				DWORD maxValueNameSize, maxValueDataSize;
				result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
				if(result != ERROR_SUCCESS)
					throw e;
				
				LPWSTR valueName = new WCHAR[maxValueNameSize];
				LPBYTE valueData = new BYTE[maxValueDataSize];
				DWORD valueIndex = 0;
				DWORD valueNameSize, valueDataSize, valueType;
				std::wstring fontFile;
				
				do {
					
					fontFile.clear();
					valueDataSize = maxValueDataSize;
					valueNameSize = maxValueNameSize;
					
					result = RegEnumValueW(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
					valueIndex ++;
					
					if(result != ERROR_SUCCESS || valueType != REG_SZ)
						continue;
					
					std::wstring wsValueName(valueName, valueNameSize);
					
					if(_wcsnicmp(faceName.c_str(), wsValueName.c_str(), faceName.length()) == 0)
					{
						fontFile.assign((LPWSTR)valueData, valueDataSize);
						break;
					}
					
				} while(result != ERROR_NO_MORE_ITEMS);
				
				delete [] valueName;
				delete [] valueData;
				
				RegCloseKey(hKey);
				
				if(fontFile.empty())
					throw Exception(Exception::Type::GenericException, "No font with name " + name);
				
				WCHAR winDir[MAX_PATH];
				::GetWindowsDirectoryW(winDir, MAX_PATH);
				
				std::wstringstream ss;
				ss << winDir << "\\Fonts\\" << fontFile;
				
				fontFile = ss.str();
				path     = std::string(fontFile.begin(), fontFile.end());
#endif
			}
			
			_fontPath = path;
			
			// Find the correct face
			FT_Long flags = static_cast<FT_Long>(_descriptor.style); // FontStyle has the same values as FT_STYLE_FLAG_XXX
			FT_Long faces = 0;
			
			FT_Init_FreeType(&_internals->library);
			FT_New_Face(_internals->library, _fontPath.c_str(), 0, &_internals->face);
			
			faces = _internals->face->num_faces;
			
			// Some fonts have multiple font faces, with different features.
			// Helvetica on OS X for example, is made out of one file with all features (italics, bold, etc) in different faces
			// We can't tell FreeType which features we want, so we have to check all faces and see if they have what we need
			if(faces > 0)
			{
				for(size_t i=0; i<faces; i++)
				{
					FT_Done_Face(_internals->face);
					FT_New_Face(_internals->library, _fontPath.c_str(), i, &_internals->face);
					
					if(_internals->face->style_flags == flags)
					{
						_faceIndex = i;
						break;
					}
				}
			}
			
			DropInternals();
		}
		
		// ---------------------
		// MARK: -
		// MARK: FreeType related
		// ---------------------
		
		void Font::InitializeInternals()
		{
			FT_Init_FreeType(&_internals->library);
			
			FT_UInt hres = 64;
			FT_Matrix matrix;
			
			matrix.xx = (FT_Fixed)((1.0f / hres) * 0x10000L);
			matrix.xy = 0.0f;
			matrix.yx = 0.0f;
			matrix.yy = (FT_Fixed)(1.0f * 0x10000L);
			
			FT_New_Face(_internals->library, _fontPath.c_str(), _faceIndex, &_internals->face);
			
			FT_Select_Charmap(_internals->face, FT_ENCODING_UNICODE);
			FT_Set_Char_Size(_internals->face, (int)((_size * _scale) * 64.0f), 0, 72 * hres, 72);
			
			FT_Set_Transform(_internals->face, &matrix, 0);
		}
		
		void Font::DropInternals()
		{
			FT_Done_Face(_internals->face);
			FT_Done_FreeType(_internals->library);
		}
		
		float Font::ConvertFontUnit(float unit) const
		{
			return unit * _size / _unitsPerEM;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Glyph handling
		// ---------------------
		
		void Font::RenderGlyph(UniChar character)
		{
			FT_Int32 flags = FT_LOAD_RENDER;
			FT_Error error;
			
			FT_UInt glyphIndex = FT_Get_Char_Index(_internals->face, character);
			
			if(_descriptor.filtering)
			{
				FT_Library_SetLcdFilter(_internals->library, FT_LCD_FILTER_LIGHT);
				FT_Library_SetLcdFilterWeights(_internals->library, _filterWeights);
				
				flags |= FT_LOAD_TARGET_LCD;
			}
			
			flags |= (_descriptor.hinting) ? FT_LOAD_FORCE_AUTOHINT : FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
			
			error = FT_Load_Glyph(_internals->face, glyphIndex, flags);
			FT_Render_Glyph(_internals->face->glyph, FT_RENDER_MODE_NORMAL);
			
			FT_GlyphSlot slot = _internals->face->glyph;
			FT_Bitmap bitmap  = slot->bitmap;
			
			uint32 width  = (bitmap.pixel_mode == FT_PIXEL_MODE_LCD) ? bitmap.width / 3 : bitmap.width;
			uint32 height = bitmap.rows;

			uint8 *data;
			
			switch(bitmap.pixel_mode)
			{
				case FT_PIXEL_MODE_MONO:
					data = new uint8[width * height];
					
					for(uint32 y=0; y<height; y++)
					{
						uint8 *source = bitmap.buffer + (y * bitmap.pitch);
						uint8 *dest   = data + width * 4 * y;
						
						for(uint32 x=0; x<width; x++)
						{
							dest[x * 4] = (source[x / 8] & (0x80 >> (x & 7))) ? 255 : 0;
						}
					}
					
					break;
					
				case FT_PIXEL_MODE_GRAY:
					data = new uint8[width * height];
					
					for(uint32 y=0; y<height; y++)
					{
						uint8 *source = bitmap.buffer + (y * bitmap.pitch);
						uint8 *dest   = data + width * y;
						
						std::copy(source, source + width, dest);
					}
					
					break;
					
				case FT_PIXEL_MODE_LCD:
					data = new uint8[bitmap.width * height];
					
					for(uint32 y=0; y<height; y++)
					{
						uint8 *source = bitmap.buffer + (y * bitmap.pitch);
						uint8 *dest = data + (y * bitmap.width);
						
						std::copy(source, source + bitmap.width, dest);
					}
					
					break;
				
				default:
					throw Exception(Exception::Type::InconsistencyException, "FreeType pixel mode unsupported!");
					return;
			}
			
			Rect rect = _texture->AllocateRegion(width + 1, height + 1);
			rect.width  -= 1.0f;
			rect.height -= 1.0f;

			_texture->SetRegionData(rect, data, GetFiltering() ? Texture::Format::RGB888 : Texture::Format::R8);
			delete [] data;
			
			Glyph glyph;
			glyph._character = character;
			glyph._region = rect;

			glyph._region.width  /= _scale;
			glyph._region.height /= _scale;
			
			glyph._offset_x = slot->bitmap_left / _scale;
			glyph._offset_y = slot->bitmap_top / _scale;
			
			glyph._u0 = rect.x / _texture->GetWidth();
			glyph._v0 = rect.y / _texture->GetHeight();
			glyph._u1 = (rect.x + rect.width) / _texture->GetWidth();
			glyph._v1 = (rect.y + rect.height) / _texture->GetHeight();
			
			FT_Load_Glyph(_internals->face, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
			slot = _internals->face->glyph;
			
			glyph._advance_x = (slot->advance.x / 64.0f) / _scale;
			glyph._advance_y = (slot->advance.y / 64.0f) / _scale;
			
			_glyphs.insert(std::unordered_map<UniChar, Glyph>::value_type(glyph._character, glyph));
		}
		
		void Font::UpdateAtlas()
		{
			if(_textureTag == _texture->GetTag())
				return;
			
			for(auto temp : _glyphs)
			{
				Glyph& glyph = temp.second;
				
				float width  = glyph._region.width  * _scale;
				float height = glyph._region.height * _scale;
				
				glyph._u0 = glyph._region.x / _texture->GetWidth();
				glyph._v0 = glyph._region.y / _texture->GetHeight();
				glyph._u1 = (glyph._region.x + width)  / _texture->GetWidth();
				glyph._v1 = (glyph._region.y + height) / _texture->GetHeight();
			}
			
			_textureTag = _texture->GetTag();
		}
		
		void Font::UpdateKerning()
		{
			for(auto i=_glyphs.begin(); i!=_glyphs.end(); i++)
			{
				FT_UInt glyphIndex = FT_Get_Char_Index(_internals->face, i->first);
				
				for(auto j=_glyphs.begin(); j!=_glyphs.end(); j++)
				{
					if(j->second._kerning.find(j->first) != j->second._kerning.end())
						continue;
					
					FT_Vector kerning;
					FT_UInt otherIndex = FT_Get_Char_Index(_internals->face, j->first);
					
					FT_Get_Kerning(_internals->face, otherIndex, glyphIndex, FT_KERNING_UNFITTED, &kerning);
					
					if(kerning.x)
					{
						float value = kerning.x / (64.0f * 64.0f);
						i->second._kerning.insert(std::unordered_map<UniChar, float>::value_type(j->first, value));
					}
				}
			}
		}
		
		void Font::RenderGlyphsFromString(String *string)
		{
			InitializeInternals();
			
			bool addedGlyphs = false;
			
			for(uint32 i=0; i<string->GetLength(); i++)
			{
				UniChar character = string->GetCharacterAtIndex(i);
				
				if(CodePoint(character).IsNewline())
					continue;
				
				if(_glyphs.find(character) != _glyphs.end())
					continue;
				
				RenderGlyph(character);
				addedGlyphs = true;
			}
			
			if(addedGlyphs)
				UpdateKerning();
			
			UpdateAtlas();
			DropInternals();
		}
		
		const Glyph& Font::GetGlyphForCharacter(UniChar character)
		{
			auto iterator = _glyphs.find(character);
			if(iterator == _glyphs.end())
			{
				InitializeInternals();
				
				RenderGlyph(character);
				UpdateKerning();
				
				UpdateAtlas();
				DropInternals();
				return _glyphs.at(character);
			}
			
			return iterator->second;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Helper
		// ---------------------
		
		float Font::GetDefaultLineHeight() const
		{
			float leading = _leading;
			float lineHeight;
			
			if(leading < 0.0f)
				leading = 0.0f;
			
			leading = floorf(leading + 0.5f);
			lineHeight = floorf(_ascent + 0.5f) - floor(_descent * 0.5f) + leading;
			
			float ascenderDelta = (leading > 0.0f) ? 0.0f : floorf(0.2f * lineHeight + 0.5f);
			return lineHeight + ascenderDelta;
		}
	}
}

