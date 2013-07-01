//
//  RNUITypesetter.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TYPESETTER_H__
#define __RAYNE_TYPESETTER_H__

#include "RNBase.h"
#include "RNAttributedString.h"
#include "RNUITextStyle.h"
#include "RNUIFont.h"

#define kRNTypesetterFontAttribute  RNSTR("kRNTypesetterFontAttribute")
#define kRNTypesetterColorAttribute RNSTR("kRNTypesetterColorAttribute")

namespace RN
{
	namespace UI
	{
		class Line;
		class Typesetter
		{
		public:
			friend class Line;
			
			Typesetter(AttributedString *string, const Rect& frame);
			~Typesetter();
			
			void SetText(AttributedString *string);
			void SetAlignment(TextAlignment alignment);
			void SetLineBreak(LineBreakMode lineBreak);
			void SetMaximumLines(uint32 maxLines);
			void SetFrame(const Rect& frame);
			
			Vector2 Dimensions();
			
		private:
			void Clear();
			void LayoutText();
			
			static Font *FontForAttributes(Dictionary *attributes);
			
			AttributedString *_string;
			TextAlignment _alignment;
			LineBreakMode _lineBreak;
			uint32 _maxLines;
			Rect _frame;
			
			bool _dirty;
			std::vector<Line *> _lines;
		};
		
		class LineSegment
		{
		public:
			LineSegment();
			LineSegment(const LineSegment& other);
			LineSegment(LineSegment&& other);
			~LineSegment();
			
			LineSegment& operator= (const LineSegment& other);
			LineSegment& operator= (LineSegment&& other);
			
			void SetFont(Font *font);
			
			void AddGlyph(const Glyph& glyph);
			void AddGlyphs(const std::vector<Glyph>& glyphs);
			void InsertGlyph(const Glyph& glyph);
			
			LineSegment SegmentWithWidth(float width, bool reverse);
			
			bool IsValid() const { return !_glyphs.empty(); }
			
			Font *GlyphFont() const { return _font; }
			const std::vector<Glyph>& Glyphs() const { return _glyphs; }
			const Vector2& Extents() const { return _extents; }

		private:
			Font *_font;
			std::vector<Glyph> _glyphs;
			Vector2 _extents;
		};
		
		class Line
		{
		public:
			friend class Typesetter;
			
			Line(Typesetter *typesetter, const Range& range);
			~Line();
		
			void Truncate(float width, TextTruncation truncation, UniChar token);
			void RemoveTruncation();
			
		private:
			void LayoutLine();
			void UpdateExtents();
			float TokenWidthInSegment(const LineSegment& segment);
			
			Typesetter *_typesetter;
			Range _range;
			
			Vector2 _extents;
			std::vector<LineSegment> _segments;
			
			bool _truncated;
			float _truncationWidth;
			UniChar _truncationToken;
			TextTruncation _truncationType;
		};
	}
}

#endif /* __RAYNE_TYPESETTER_H__ */
