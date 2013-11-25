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
			
			Typesetter(AttributedString *string, const Rect& frame);
			~Typesetter();
			
			void SetText(AttributedString *string);
			void SetAlignment(TextAlignment alignment);
			void SetLineBreak(LineBreakMode lineBreak);
			void SetMaximumLines(uint32 maxLines);
			void SetAllowPartiallyClippedLined(bool allowClippedLines);
			void SetFrame(const Rect& frame);
			void SetEllipsis(UniChar character);
			
			void InvalidateStringInRange(const Range& range);
			
			Vector2 GetDimensions();
			Vector2 GetVisibleDimensions();
			
			Model *GetLineModel();
			
			Line *GetLineAtPoint(const Vector2& point);
			Range GetRangeOfCharactersAtPoint(const Vector2& point);
			AttributedString *GetText() const { return _string; }
			
			const std::vector<Line *>& GetLines();
			const std::vector<Line *>& GetVisibleLines();
			
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
			LineSegment();
			LineSegment(const LineSegment& other);
			LineSegment(LineSegment&& other);
			~LineSegment();
			
			LineSegment& operator= (const LineSegment& other);
			LineSegment& operator= (LineSegment&& other);
			
			LineSegment SegmentWithWidth(float width, bool reverse);
			
			
			void SetFont(Font *font);
			void SetColor(const RN::Color& color);
			void SetOffset(const Vector2& offset);
			void SetRangeOffset(size_t offset);
			
			void AppendGlyph(const Glyph& glyph);
			void PrependGlyph(const Glyph& glyph);
			
			bool CanMergeWithSegment(const LineSegment& other);
			bool IsValid() const { return !_glyphs.empty(); }
			
			const std::vector<Glyph>& GetGlyphs() const { return _glyphs; }
			const Vector2& GetExtents() const { return _extents; }
			const Vector2& GetOffset() const { return _offset; }
			const RN::Color& GetColor() const { return _color; }
			const Range& GetRange() const { return _range; }
			Range GetRangeOfCharactersAtPoint(const Vector2& point) const;
			
			Font *GetFont() const { return _font; }
			Rect GetFrame() const { return Rect(_offset, _extents); }
			
			void CreateGlyphMesh(Vector2 *vertices, Vector2 *uvCoords, uint16 *indices, size_t offset);
			
		private:
			void AppendGlyphs(const std::vector<Glyph>& glyphs);
			bool IsValidGlyph(const Glyph& glyph) const;
			
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
			
			Line(AttributedString *string, const Range& range);
			~Line();
		
			void Truncate(float width, TextTruncation truncation, UniChar token);
			void SetLineOffset(const Vector2& offset);
			
			const std::vector<LineSegment>& GetSegments();
			const Vector2& GetExtents();
			const Vector2& GetUntruncatedExtents();
			Range GetRangeOfCharactersAtPoint(const Vector2& point);
			
			Rect GetFrame();
			const Range GetRange() const { return _range; }
			
			Array *GetMeshes(float offset);
			
		private:
			void LayoutLine();
			void UpdateExtents();
			
			Dictionary *GenerateMesh(const std::vector<LineSegment *>& segments, float offset);
			Array *GenerateMeshes(float offset);
			
			float TokenWidthInSegment(const LineSegment& segment);
			
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
