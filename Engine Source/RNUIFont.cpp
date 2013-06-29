//
//  RNUIFont.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include "RNUIFont.h"
#include "RNPathManager.h"
#include "RNBaseInternal.h"
#include "RNKernel.h"

const char *kRNCommonCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890.,-;:_-+*/!\"§$%&()=?<>' ";

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Font)
		
		float Glyph::Kerning(UniChar character) const
		{
			auto iterator = _kerning.find(character);
			return (iterator != _kerning.end()) ? iterator->second : 0.0f;
		}
		

		struct FontInternals
		{
			FT_Library library;
			FT_Face face;
		};
		
		
#define _internals (reinterpret_cast<FontInternals *>(_finternals))
		
		Font::Font(const std::string& name, float size) :
			Font(name, size, FontDescriptor())
		{}
		
		Font::Font(const std::string& name, float size, const FontDescriptor& descriptor) :
			_descriptor(descriptor)
		{
			TextureParameter parameter;
			
			parameter.mipMaps = 0;
			parameter.generateMipMaps = false;
			parameter.format = TextureParameter::Format::R8;
			parameter.wrapMode = TextureParameter::WrapMode::Clamp;
			parameter.filter = TextureParameter::Filter::Nearest;
			
			_scale = Kernel::SharedInstance()->ScaleFactor();
			_texture = new TextureAtlas(4096 * _scale, 4096 * _scale, parameter);
			
			_finternals = 0;
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
			DropInternals();
		}
		
		void Font::ResolveFontName(const std::string& name)
		{
			std::string path;
			
			try
			{
				path = PathManager::PathForName(name);
			}
			catch(ErrorException e)
			{
#if RN_PLATFORM_MAC_OS
				@autoreleasepool
				{
					CTFontSymbolicTraits traits = 0;
					traits |= (_descriptor.style & FontDescriptor::FontStyleBold) ? kCTFontTraitBold : 0;
					traits |= (_descriptor.style & FontDescriptor::FontStyleItalic) ? kCTFontTraitItalic : 0;
					
					CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, name.c_str(), kCFStringEncodingASCII);
					CTFontRef source = CTFontCreateWithName(fontName, _size, nullptr);
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
			}
			
			_fontPath = path;
			
			// Find the correct face
			_finternals = new FontInternals;
			
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
			
			FT_New_Face(_internals->library, _fontPath.c_str(), _faceIndex, &_internals->face);
			
			FT_Select_Charmap(_internals->face, FT_ENCODING_UNICODE);
			FT_Set_Char_Size(_internals->face, (int)((_size * _scale) * 64.0f), 0, 72 * hres, 72);
			
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
			
			uint32 width  = bitmap.width;
			uint32 height = bitmap.rows;

			uint8 *data = new uint8[width * height];
			
			switch(bitmap.pixel_mode)
			{
				case FT_PIXEL_MODE_MONO:
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
					for(uint32 y=0; y<height; y++)
					{
						uint8 *source = bitmap.buffer + (y * bitmap.pitch);
						uint8 *dest   = data + width * y;
						
						for(uint32 x=0; x<width; x++)
						{
							*dest ++ = *source ++;
						}
					}
					
					break;
					
				default:
					throw ErrorException(0);
					return;
			}
			
			Rect rect = _texture->AllocateRegion(width + 1, height + 1);
			rect.width  -= 1.0f;
			rect.height -= 1.0f;
			
			_texture->SetRegionData(rect, data, TextureParameter::Format::R8);
			delete [] data;
			
			Glyph glyph;
			glyph._character = character;
			glyph._region    = rect;
			glyph._region.width  /= _scale;
			glyph._region.height /= _scale;
			
			glyph._offset_x = slot->bitmap_left / _scale;
			glyph._offset_y = slot->bitmap_top  / _scale;
			
			glyph._u0 = rect.x / _texture->Width();
			glyph._v0 = rect.y / _texture->Height();
			glyph._u1 = (rect.x + rect.width) / _texture->Width();
			glyph._v1 = (rect.y + rect.height) / _texture->Height();
			
			FT_Load_Glyph(_internals->face, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
			slot = _internals->face->glyph;
			
			glyph._advance_x = (slot->advance.x / 64.0f) / _scale;
			glyph._advance_y = (slot->advance.y / 64.0f) / _scale;
			
			_glyphs.insert(std::unordered_map<UniChar, Glyph>::value_type(glyph._character, glyph));
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
			
			for(uint32 i=0; i<string->Length(); i++)
			{
				UniChar character = string->CharacterAtIndex(i);
				
				if(CodePoint(character).IsNewline())
					continue;
				
				if(_glyphs.find(character) != _glyphs.end())
					continue;
				
				RenderGlyph(character);
				addedGlyphs = true;
			}
			
			if(addedGlyphs)
				UpdateKerning();
			
			DropInternals();
		}
		
		const Glyph& Font::GlyphForCharacter(UniChar character)
		{
			auto iterator = _glyphs.find(character);
			if(iterator == _glyphs.end())
			{
				RenderGlyph(character);
				UpdateKerning();
				
				return _glyphs.at(character);
			}
			
			return iterator->second;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Helper
		// ---------------------
		
		float Font::DefaultLineHeight() const
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
		
		
		
		float Font::WidthOfString(String *string)
		{
			float offsetX = 0.0f;
			
			for(size_t i=0; i<string->Length(); i++)
			{
				UniChar character = string->CharacterAtIndex(static_cast<uint32>(i));
				Glyph& glyph = _glyphs.at(character);
				
				offsetX += glyph.AdvanceX();
			}
			
			return offsetX;
		}
		
		Vector2 Font::SizeOfString(String *string, const TextStyle& style)
		{
			RenderGlyphsFromString(string);
			
			float offsetX = 0.0f;
			size_t lines = 1;
			
			for(size_t i=0; i<string->Length(); i++)
			{
				UniChar character = string->CharacterAtIndex(static_cast<uint32>(i));
				Glyph& glyph = _glyphs.at(character);
				
				offsetX += glyph.AdvanceX();
			}
				
			return Vector2(offsetX, lines * DefaultLineHeight());
		}
		
		void Font::AlignLine(Vector2 *begin, Vector2 *end, const TextStyle& style)
		{
			if(begin == end)
				return;
			
			float width  = (end - 4)->x - (begin + 1)->x;
			float offset;
			
			switch(style.alignment)
			{
				case TextAlignment::Left:
					return;
					
				case TextAlignment::Center:
					offset = (style.size.x * 0.5f) - (width * 0.5f);
					break;
					
				case TextAlignment::Right:
					offset = roundf(style.size.x - width);
					break;
			}
			
			while(begin < end)
			{
				begin->x += offset;
				begin ++;
			}
		}
		
		size_t Font::RenderableCharactersInString(String *string)
		{
			size_t size = 0;
			
			for(size_t i=0; i<string->Length(); i++)
			{
				UniChar character = string->CharacterAtIndex(static_cast<uint32>(i));
				
				if(CodePoint(character).IsNewline())
					continue;
				
				size ++;
			}
			
			return size;
		}
		
		Mesh *Font::RenderString(String *string, const TextStyle& style)
		{
			RenderGlyphsFromString(string);
			
			size_t visibleLength = RenderableCharactersInString(string);
			
			size_t vertexCount  = visibleLength * 4;
			size_t indicesCount = visibleLength * 6;
			
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			vertexDescriptor.elementCount  = vertexCount;
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			uvDescriptor.elementCount  = vertexCount;
			
			MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
			indicesDescriptor.elementMember = 1;
			indicesDescriptor.elementSize   = sizeof(uint16);
			indicesDescriptor.elementCount  = indicesCount;
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor, indicesDescriptor };
			Mesh *mesh = new Mesh(descriptors);
			
			Vector2 *vertices = mesh->Element<Vector2>(kMeshFeatureVertices);
			Vector2 *uvCoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
			uint16 *indices   = mesh->Element<uint16>(kMeshFeatureIndices);
			
			bool isOnNewLine = true;
			Vector2 *lineBegin = vertices;
			
#define InsertNewLine() do { \
			AlignLine(lineBegin, vertices, style); \
			\
			offsetX = 0.0f; \
			offsetY -= height; \
			\
			lineBegin = vertices; \
			isOnNewLine = true; \
		} while(0)
			
			// Generate a mesh for each glyph
			float offsetX = 0.0f;
			float offsetY = 0.0f;
			
			size_t iOffset = 0;
			float height = DefaultLineHeight();
			
			for(size_t i=0; i<string->Length(); i++)
			{
				UniChar character = string->CharacterAtIndex(static_cast<uint32>(i));
				if(CodePoint(character).IsNewline())
				{
					InsertNewLine();
					continue;
				}
				
				Glyph& glyph = _glyphs.at(character);
				
				float x0 = offsetX + glyph.OffsetX();
				float y1 = offsetY + glyph.OffsetY() + (style.size.y - height);
				float x1 = x0 + glyph.Width();
				float y0 = y1 - glyph.Height();
				
				if(!isOnNewLine)
				{
					if(style.kerning)
					{
						UniChar previous = string->CharacterAtIndex(static_cast<uint32>(i - 1));
						float kerning = glyph.Kerning(previous);
						
						offsetX += kerning;
						x0 += kerning;
					}
					
					if(style.constraintWidth && x0 >= style.size.x)
					{
						x0 = glyph.OffsetX();
						x1 = glyph.Width();
						
						y0 -= height;
						y1 -= height;
						
						InsertNewLine();
					}
				}
				
				*vertices ++ = Vector2(x1, y1);
				*vertices ++ = Vector2(x0, y1);
				*vertices ++ = Vector2(x1, y0);
				*vertices ++ = Vector2(x0, y0);
				
				*uvCoords ++ = Vector2(glyph._u1, glyph._v0);
				*uvCoords ++ = Vector2(glyph._u0, glyph._v0);
				*uvCoords ++ = Vector2(glyph._u1, glyph._v1);
				*uvCoords ++ = Vector2(glyph._u0, glyph._v1);
				
				*indices ++ = (iOffset * 4) + 0;
				*indices ++ = (iOffset * 4) + 1;
				*indices ++ = (iOffset * 4) + 2;
				
				*indices ++ = (iOffset * 4) + 1;
				*indices ++ = (iOffset * 4) + 3;
				*indices ++ = (iOffset * 4) + 2;
				
				offsetX += glyph.AdvanceX();
				
				isOnNewLine = false;
				iOffset ++;
			}
			
			AlignLine(lineBegin, vertices, style);
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->ReleaseElement(kMeshFeatureUVSet0);
			mesh->ReleaseElement(kMeshFeatureIndices);
			mesh->UpdateMesh();
			
			return mesh->Autorelease();
			
#undef InsertNewLine
		}
	}
	
#undef _internals
}

