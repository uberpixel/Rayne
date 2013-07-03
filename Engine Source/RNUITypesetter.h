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
#include "RNModel.h"

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
			Model *LineModel();
			
			const std::vector<Line *>& Lines();
			const std::vector<Line *>& VisibleLines();
			
		private:
			void Clear();
			void CalculateVisibleLines();
			void LayoutText();
			void MergeMeshes();
			
			static Font *FontForAttributes(Dictionary *attributes);
			
			AttributedString *_string;
			TextAlignment _alignment;
			LineBreakMode _lineBreak;
			uint32 _maxLines;
			Rect _frame;
			
			bool _dirty;
			bool _frameChanged;
			std::vector<Line *> _lines;
			std::vector<Line *> _visibleLines;
			
			Model *_model;
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
			void SetOffset(const Vector2& offset);
			
			void AddGlyph(const Glyph& glyph);
			void AddGlyphs(const std::vector<Glyph>& glyphs);
			void InsertGlyph(const Glyph& glyph);
			
			LineSegment SegmentWithWidth(float width, bool reverse);
			
			bool IsValid() const { return !_glyphs.empty(); }
			
			Font *GlyphFont() const { return _font; }
			const std::vector<Glyph>& Glyphs() const { return _glyphs; }
			const Vector2& Extents() const { return _extents; }
			const Vector2& Offset() const { return _offset; }
			
			void CreateGlyphMesh(Vector2 *vertices, Vector2 *uvCoords, uint16 *indices, size_t offset);
			
		private:
			bool IsValidGlyph(const Glyph& glyph) const;
			
			Font *_font;
			std::vector<Glyph> _glyphs;
			
			Vector2 _offset;
			Vector2 _extents;
		};
		
		class Line
		{
		public:
			Line(AttributedString *string, const Range& range);
			~Line();
		
			void Truncate(float width, TextTruncation truncation, UniChar token);
			void SetLineOffset(float offset);
			
			const std::vector<LineSegment>& Segments();
			const Vector2& Extents();
			Dictionary *Meshes();
			
		private:
			void LayoutLine();
			void UpdateExtents();
			
			void GenerateMesh(Dictionary *meshes, Font *font, const std::vector<LineSegment *>& segments);
			Dictionary *GenerateMeshes();
			
			float TokenWidthInSegment(const LineSegment& segment);
			
			AttributedString *_string;
			Range _range;
			
			float _offset;
			Vector2 _extents;
			
			std::vector<LineSegment> _segments;
			
			bool _dirty;
			bool _truncated;
			float _truncationWidth;
			UniChar _truncationToken;
			TextTruncation _truncationType;
		};
	}
}

#endif /* __RAYNE_TYPESETTER_H__ */
