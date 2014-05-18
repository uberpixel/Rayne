//
//  RNUITypesetter.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TYPESETTER_H__
#define __RAYNE_TYPESETTER_H__

#include "RNBase.h"
#include "RNAttributedString.h"
#include "RNUITextStyle.h"
#include "RNUIFont.h"
#include "RNUIColor.h"
#include "RNModel.h"

#define kRNTypesetterFontAttribute  RNCSTR("kRNTypesetterFontAttribute")
#define kRNTypesetterColorAttribute RNCSTR("kRNTypesetterColorAttribute")

namespace RN
{
	namespace UI
	{
		class Line;
		class Typesetter
		{
		public:
			friend class Line;
			
			RNAPI Typesetter(AttributedString *string, const Rect &frame);
			RNAPI ~Typesetter();
			
			RNAPI void SetText(AttributedString *string);
			RNAPI void SetAlignment(TextAlignment alignment);
			RNAPI void SetLineBreak(LineBreakMode lineBreak);
			RNAPI void SetMaximumLines(uint32 maxLines);
			RNAPI void SetAllowPartiallyClippedLined(bool allowClippedLines);
			RNAPI void SetFrame(const Rect &frame);
			RNAPI void SetEllipsis(UniChar character);
			
			RNAPI void InvalidateStringInRange(const Range &range);
			
			RNAPI Vector2 GetDimensions();
			RNAPI Vector2 GetVisibleDimensions();
			
			RNAPI Model *GetLineModel();
			
			RNAPI Line *GetLineAtPoint(const Vector2 &point);
			RNAPI Range GetRangeOfCharactersAtPoint(const Vector2 &point);
			RNAPI AttributedString *GetText() const { return _string; }
			
			RNAPI const std::vector<Line *>& GetLines();
			RNAPI const std::vector<Line *>& GetVisibleLines();
			
		private:
			void Clear();
			void CalculateVisibleLines();
			void LayoutText();
			void MergeMeshes();
			Mesh *DequeMesh(size_t vertices);
			
			static Font *FontForAttributes(Dictionary *attributes);
			static const RN::Color& ColorForAttributes(Dictionary *attributes);
			
			AttributedString *_string;
			TextAlignment _alignment;
			LineBreakMode _lineBreak;
			uint32 _maxLines;
			Rect _frame;
			
			UniChar _ellipsis;
			
			bool _dirty;
			bool _allowClippedLines;
			bool _frameChanged;
			std::vector<Line *> _lines;
			std::vector<Line *> _visibleLines;
			
			Array _meshes;
			Model *_model;
		};
		
		class LineSegment
		{
		public:
			RNAPI LineSegment();
			RNAPI LineSegment(const LineSegment &other);
			RNAPI LineSegment(LineSegment&& other);
			RNAPI ~LineSegment();
			
			RNAPI LineSegment &operator= (const LineSegment &other);
			RNAPI LineSegment &operator= (LineSegment&& other);
			
			RNAPI LineSegment SegmentWithWidth(float width, bool reverse);
			
			
			RNAPI void SetFont(Font *font);
			RNAPI void SetColor(const RN::Color& color);
			RNAPI void SetOffset(const Vector2 &offset);
			RNAPI void SetRangeOffset(size_t offset);
			
			RNAPI void AppendGlyph(const Glyph &glyph);
			RNAPI void PrependGlyph(const Glyph &glyph);
			
			RNAPI bool CanMergeWithSegment(const LineSegment &other);
			RNAPI bool IsValid() const { return !_glyphs.empty(); }
			
			RNAPI const std::vector<Glyph>& GetGlyphs() const { return _glyphs; }
			RNAPI const Vector2 &GetExtents() const { return _extents; }
			RNAPI const Vector2 &GetOffset() const { return _offset; }
			RNAPI const RN::Color& GetColor() const { return _color; }
			RNAPI const Range &GetRange() const { return _range; }
			RNAPI Range GetRangeOfCharactersAtPoint(const Vector2 &point) const;
			
			RNAPI Font *GetFont() const { return _font; }
			RNAPI Rect GetFrame() const { return Rect(_offset, _extents); }
			
			RNAPI void CreateGlyphMesh(Vector2 *vertices, Vector2 *uvCoords, uint16 *indices, size_t offset);
			
		private:
			void AppendGlyphs(const std::vector<Glyph>& glyphs);
			bool IsValidGlyph(const Glyph &glyph) const;
			
			Font *_font;
			std::vector<Glyph> _glyphs;
			
			Vector2 _offset;
			Vector2 _extents;
			RN::Color _color;
			Range _range;
		};
		
		class Line
		{
		public:
			friend class Typesetter;
			
			RNAPI Line(AttributedString *string, const Range &range);
			RNAPI ~Line();
		
			RNAPI void Truncate(float width, TextTruncation truncation, UniChar token);
			RNAPI void SetLineOffset(const Vector2 &offset);
			
			RNAPI const std::vector<LineSegment>& GetSegments();
			RNAPI const Vector2 &GetExtents();
			RNAPI const Vector2 &GetUntruncatedExtents();
			RNAPI Range GetRangeOfCharactersAtPoint(const Vector2 &point);
			
			RNAPI Rect GetFrame();
			RNAPI const Range GetRange() const { return _range; }
			
			RNAPI Array *GetMeshes(float offset);
			
		private:
			void LayoutLine();
			void UpdateExtents();
			
			Dictionary *GenerateMesh(const std::vector<LineSegment *>& segments, float offset);
			Array *GenerateMeshes(float offset);
			
			float TokenWidthInSegment(const LineSegment &segment);
			
			Typesetter *_typesetter;
			AttributedString *_string;
			Range _range;
			
			Vector2	_offset;
			Vector2 _extents;
			Vector2 _untruncatedExtents;
			
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
