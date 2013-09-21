//
//  RNUIFont.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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

namespace RN
{
	namespace UI
	{
		class Font;
		class Glyph
		{
		friend class Font;
		public:
			UniChar GetCharacter() const { return _character; }
			float GetKerning(UniChar character) const; // Returns the kerning for this glyph rendered after the given character
			
			float GetWidth() const  { return _region.width; }
			float GetHeight() const { return _region.height; }
			
			float GetOffsetX() const { return _offset_x; }
			float GetOffsetY() const { return _offset_y; }
			
			float GetAdvanceX() const { return _advance_x; }
			float GetAdvanceY() const { return _advance_y; }
			
			float GetU0() const { return _u0; }
			float GetU1() const { return _u1; }
			float GetV0() const { return _v0; }
			float GetV1() const { return _v1; }
			
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
		
		class Font : public Object
		{
		friend class Glyph;
		public:
			Font(const std::string& name, float size);
			Font(const std::string& name, float size, const FontDescriptor& descriptor);
			~Font() override;
			
			static Font *WithName(const std::string& name, float size);
			static Font *WithNameAndDescriptor(const std::string& name, float size, const FontDescriptor& descriptor);
			
			bool GetHinting() const { return _descriptor.hinting; }
			bool GetFiltering() const { return _descriptor.filtering; };
			
			float GetAscent() const { return _ascent; }
			float GetDescent() const { return _descent; }
			float GetLeading() const { return _leading; }
			float GetSize() const { return _size; }
			float GetUnitsPerEM() const { return _unitsPerEM; }
			float GetDefaultLineHeight() const;
			
			Texture *GetTexture() const { return _texture; }
			
			const Glyph& GetGlyphForCharacter(UniChar character);
			void RenderGlyphsFromString(String *string);
			
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
			void *_finternals;
			size_t _faceIndex;
			
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
			
			RNDefineMeta(Font, Object)
		};
	}
}

#endif /* __RAYNE_UIFONT_H__ */
