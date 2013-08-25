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
			UniChar Character() const { return _character; }
			float Kerning(UniChar character) const; // Returns the kerning for this glyph rendered after the given character
			
			float Width() const  { return _region.width; }
			float Height() const { return _region.height; }
			
			int OffsetX() const { return _offset_x; }
			int OffsetY() const { return _offset_y; }
			
			float AdvanceX() const { return _advance_x; }
			float AdvanceY() const { return _advance_y; }
			
			float U0() const { return _u0; }
			float U1() const { return _u1; }
			float V0() const { return _v0; }
			float V1() const { return _v1; }
			
		private:
			UniChar _character;
			Rect _region;
			
			int _offset_x;
			int _offset_y;
			
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
			
			bool Hinting() const { return _descriptor.hinting; }
			bool Filtering() const { return _descriptor.filtering; };
			
			float Ascent() const { return _ascent; }
			float Descent() const { return _descent; }
			float Leading() const { return _leading; }
			float Size() const { return _size; }
			float UnitsPerEM() const { return _unitsPerEM; }
			float DefaultLineHeight() const;
			
			Texture *Texture() const { return _texture; }
			
			const Glyph& GlyphForCharacter(UniChar character);
			void RenderGlyphsFromString(String *string);
			
		private:
			void ResolveFontName(const std::string& name);
			void Initialize();
			void InitializeInternals();
			void DropInternals();
			
			void RenderGlyph(UniChar character);
			void UpdateKerning();
			
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
			
			RNDefineMeta(Font, Object)
		};
	}
}

#endif /* __RAYNE_UIFONT_H__ */
