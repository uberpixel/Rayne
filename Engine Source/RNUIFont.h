//
//  RNUIFont.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIFONT_H__
#define __RAYNE_UIFONT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTextureAtlas.h"
#include "RNArray.h"
#include "RNString.h"
#include "RNMesh.h"
#include "RNUITextStyle.h"

#define kRNUIFontInvalidateGlyphsMessage RNCSTR("kRNUIFontInvalidateGlyphsMessage")

namespace RN
{
	namespace UI
	{
		class Font;
		class Glyph
		{
		friend class Font;
		public:
			RNAPI UniChar GetCharacter() const { return _character; }
			RNAPI float GetKerning(UniChar character) const; // Returns the kerning for this glyph rendered after the given character
			
			RNAPI float GetWidth() const  { return _region.width; }
			RNAPI float GetHeight() const { return _region.height; }
			
			RNAPI float GetOffsetX() const { return _offset_x; }
			RNAPI float GetOffsetY() const { return _offset_y; }
			
			RNAPI float GetAdvanceX() const { return _advance_x; }
			RNAPI float GetAdvanceY() const { return _advance_y; }
			
			RNAPI float GetU0() const { return _u0; }
			RNAPI float GetU1() const { return _u1; }
			RNAPI float GetV0() const { return _v0; }
			RNAPI float GetV1() const { return _v1; }
			
		private:
			UniChar _character;
			Rect _region;
			
			float _offset_x;
			float _offset_y;
			
			float _advance_x;
			float _advance_y;
			
			float _u0, _v0, _u1, _v1;
			
			std::unordered_map<UniChar, float> _kerning;
		};
		
		class FontDescriptor
		{
		public:
			enum
			{
				FontStyleItalic = (1 << 0),
				FontStyleBold = (1 << 1)
			};
			typedef uint32 FontStyle;
			
			FontDescriptor()
			{
				hinting = true;
				filtering = true;
				style = 0;
				
				textureFilter = false;
				mipMaps = false;
			}
			
			bool hinting;
			bool filtering;
			bool textureFilter;
			bool mipMaps;
			FontStyle style;
		};
		
		struct FontInternals;
		
		class Font : public Object
		{
		friend class Glyph;
		public:
			RNAPI Font(const std::string& name, float size);
			RNAPI Font(const std::string& name, float size, const FontDescriptor& descriptor);
			RNAPI ~Font() override;
			
			RNAPI static Font *WithName(const std::string& name, float size);
			RNAPI static Font *WithNameAndDescriptor(const std::string& name, float size, const FontDescriptor& descriptor);
			
			RNAPI bool GetHinting() const { return _descriptor.hinting; }
			RNAPI bool GetFiltering() const { return _descriptor.filtering; };
			
			RNAPI float GetAscent() const { return _ascent; }
			RNAPI float GetDescent() const { return _descent; }
			RNAPI float GetLeading() const { return _leading; }
			RNAPI float GetSize() const { return _size; }
			RNAPI float GetUnitsPerEM() const { return _unitsPerEM; }
			RNAPI float GetDefaultLineHeight() const;
			
			RNAPI Texture *GetTexture() const { return _texture; }
			
			RNAPI const Glyph& GetGlyphForCharacter(UniChar character);
			RNAPI void RenderGlyphsFromString(String *string);
			
		private:
			void ResolveFontName(const std::string& name);
			void Initialize();
			void InitializeInternals();
			void DropInternals();
			
			void RenderGlyph(UniChar character);
			void UpdateKerning();
			void UpdateAtlas();
			
			float ConvertFontUnit(float unit) const;
			
			std::string _fontPath;
			size_t _faceIndex;
			PIMPL<FontInternals> _internals;
			
			float _size;
			float _scale;
			float _height;
			
			float _ascent;
			float _descent;
			float _leading;
			float _unitsPerEM;
			
			FontDescriptor _descriptor;
			uint8 _filterWeights[5];
			
			TextureAtlas *_texture;
			std::unordered_map<UniChar, Glyph> _glyphs;
			uint32 _textureTag;
			
			RNDeclareMeta(Font)
		};
	}
}

#endif /* __RAYNE_UIFONT_H__ */
