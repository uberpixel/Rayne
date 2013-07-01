//
//  RNUITypesetter.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITypesetter.h"
#include "RNAlgorithm.h"
#include "RNDictionary.h"
#include "RNResourcePool.h"

namespace RN
{
	namespace UI
	{
		// ---------------------
		// MARK: -
		// MARK: Typesetter
		// ---------------------
		
		Typesetter::Typesetter(AttributedString *string, const Rect& frame) :
			_frame(frame)
		{
			RN_ASSERT(string, "String mustn't be NULL!");
			
			_string = string->Retain();
			
			_alignment = TextAlignment::Left;
			_lineBreak = LineBreakMode::CharacterWrapping;
			_maxLines  = 0;
			
			_dirty = true;
		}
		
		Typesetter::~Typesetter()
		{
			_string->Release();
			Clear();
		}
		
		Font *Typesetter::FontForAttributes(Dictionary *attributes)
		{
			static Font *defaultFont = nullptr;
			static std::once_flag flag;
			
			std::call_once(flag, [&]() {
				defaultFont = ResourcePool::SharedInstance()->ResourceWithName<Font>(kRNResourceKeyDefaultFont);
			});
			
			Font *font = attributes->ObjectForKey<Font>(kRNTypesetterFontAttribute);
			return font ? font : defaultFont;
		}
		
		
		
		void Typesetter::SetText(AttributedString *string)
		{
			RN_ASSERT(string, "String mustn't be NULL!");
			
			_string->Release();
			_string = string->Retain();
			_dirty = true;
		}
		
		void Typesetter::SetAlignment(TextAlignment alignment)
		{
			_alignment = alignment;
			_dirty = true;
		}
		
		void Typesetter::SetLineBreak(LineBreakMode lineBreak)
		{
			_lineBreak = lineBreak;
			_dirty = true;
		}
		
		void Typesetter::SetMaximumLines(uint32 lines)
		{
			_maxLines = lines;
			_dirty = true;
		}
		
		void Typesetter::SetFrame(const Rect& frame)
		{
			if(Math::FastAbs(frame.width - _frame.width) < k::EpsilonFloat && Math::FastAbs(frame.height - _frame.height) < k::EpsilonFloat)
			{
				_frameChanged = true;
				_frame = frame;
				
				return;
			}
			
			_frame = frame;
			_dirty = true;
		}
		
		
		Vector2 Typesetter::Dimensions()
		{
			LayoutText();
			
			Vector2 extents;
			for(Line *line : _lines)
			{
				extents.x = std::max(line->Extents().x, extents.x);
				extents.y += line->Extents().y;
			}
			
			return extents;
		}
		
		const std::vector<Line *>& Typesetter::Lines()
		{
			LayoutText();
			return _lines;
		}
		
		const std::vector<Line *>& Typesetter::VisibleLines()
		{
			CalculateVisibleLines();
			return _visibleLines;
		}
		
		
		
		void Typesetter::Clear()
		{
			for(auto i=_lines.begin(); i!=_lines.end(); i++)
				delete *i;
			
			_lines.clear();
			_visibleLines.clear();
		}
		
		void Typesetter::CalculateVisibleLines()
		{
			LayoutText();
			
			if(!_frameChanged)
				return;
			
			_visibleLines.clear();
			
			float offset = 0.0f;
			float height = 0.0f;
			
			for(Line *line : _lines)
			{
				float lineHeight = line->Extents().y;
				
				if(offset >= _frame.y)
				{
					_visibleLines.push_back(line);
					height += lineHeight;
					
					if(height >= _frame.height)
						break;
				}
				else
					offset += lineHeight;
			}
			
			_frameChanged = false;
		}
		
		void Typesetter::LayoutText()
		{
			if(!_dirty)
				return;
			
			Clear();
			
			size_t length  = _string->Length();
			String *string = _string->String();
			
			Range range(0, 0);
			
			float offsetX  = 0.0f;
			size_t lines   = 0;
			bool onNewLine = true;
			bool forcedLinebreak = false;
			UniChar previous;
			
			bool truncateLine = (_lineBreak == LineBreakMode::TruncateHead || _lineBreak == LineBreakMode::TruncateMiddle || _lineBreak == LineBreakMode::TruncateTail);
			TextTruncation truncation;
			
			if(truncateLine)
			{
				switch(_lineBreak)
				{
					case LineBreakMode::TruncateHead:
						truncation = TextTruncation::Start;
						break;
						
					case LineBreakMode::TruncateMiddle:
						truncation = TextTruncation::Middle;
						break;
						
					case LineBreakMode::TruncateTail:
						truncation = TextTruncation::End;
						break;
						
					default:
						break;
				}
			}
			
#define SubmitLine() { \
				Line *line = new Line(_string, range); \
				_lines.push_back(line); \
				\
				if(truncateLine) \
					line->Truncate(_frame.width, truncation, CodePoint::Ellipsis()); \
			}
			
#define LineBreak(forced) { \
				offsetX = 0.0f; \
				onNewLine = true; \
				forcedLinebreak = forced; \
				lines ++; \
				if(_maxLines > 0 && lines >= _maxLines) \
					goto finishLayout; \
			}
			
			AutoreleasePool *pool = new AutoreleasePool();
			
			for(size_t i=0; i<length; i++)
			{
				UniChar character = string->CharacterAtIndex(static_cast<uint32>(i));
				CodePoint point = CodePoint(character);
				
				if(point.IsNewline())
				{
					SubmitLine();
					LineBreak(false);
					
					range.origin += range.length + 1; // Skip over the newline character
					range.length = 0;
					continue;
				}
				
				// If there was a forced line break and we start with a white space, skip over it
				if(forcedLinebreak && point.IsWhitespace())
				{
					range.origin ++;
					forcedLinebreak = false;
					
					continue;
				}
				
				Font *font = FontForAttributes(_string->AttributesAtIndex(i));
				const Glyph& glyph = font->GlyphForCharacter(character);
				
				float width = glyph.AdvanceX();
				
				if(!onNewLine)
					width += glyph.Kerning(previous);
				
				forcedLinebreak = false;
				onNewLine = false;
				previous  = character;
				
				switch(_lineBreak)
				{
					case LineBreakMode::CharacterWrapping:
						if(offsetX + width >= _frame.width)
						{
							SubmitLine();
							LineBreak(true);
							
							range.origin += range.length;
							range.length = 0;
							
							// Re-process the character
							i --;
							continue;
						}
						
						offsetX += width;
						range.length ++;
						break;
						
					case LineBreakMode::TruncateHead:
					case LineBreakMode::TruncateMiddle:
					case LineBreakMode::TruncateTail:
						offsetX += width;
						range.length ++;
						break;
						
					default:
						throw ErrorException(0);
				}
			}
			
			if(range.origin < length && (_maxLines == 0 || lines < _maxLines))
			{
				SubmitLine();
			}
			
		finishLayout:
			delete pool;
			pool = new AutoreleasePool();
			
			for(auto i=_lines.begin(); i!=_lines.end(); i++)
			{
				Line *line = *i;
				line->Extents();
			}
			
			delete pool;
			
			_dirty = false;
			_frameChanged = true;
		}
		
		// ---------------------
		// MARK: -
		// MARK: LineSegments
		// ---------------------
		
		LineSegment::LineSegment()
		{
			_font = nullptr;
		}
		LineSegment::LineSegment(const LineSegment& other)
		{
			_font    = other._font ? other._font->Retain() : nullptr;
			_glyphs  = other._glyphs;
			_extents = other._extents;
		}
		LineSegment::LineSegment(LineSegment&& other)
		{
			_font    = other._font;
			_glyphs  = std::move(other._glyphs);
			_extents = std::move(other._extents);
			
			other._font = nullptr;
		}
		
		LineSegment::~LineSegment()
		{
			if(_font)
				_font->Release();
		}
		
		
		LineSegment& LineSegment::operator= (const LineSegment& other)
		{
			_font    = other._font ? other._font->Retain() : nullptr;
			_glyphs  = other._glyphs;
			_extents = other._extents;
			
			return *this;
		}
		LineSegment& LineSegment::operator= (LineSegment&& other)
		{
			_font    = other._font;
			_glyphs  = std::move(other._glyphs);
			_extents = std::move(other._extents);
			
			other._font = nullptr;
			return *this;
		}
		
		void LineSegment::SetFont(Font *font)
		{
			if(_font)
				_font->Release();
			
			_font = font->Retain();
			_extents.y = _font->DefaultLineHeight();
		}
		
		void LineSegment::AddGlyph(const Glyph& glyph)
		{
			_glyphs.push_back(glyph);
			_extents.x += glyph.AdvanceX();
			
			if(_glyphs.size() > 1)
				_extents.x += glyph.Kerning(_glyphs[_glyphs.size() - 2].Character());
		}
		
		void LineSegment::AddGlyphs(const std::vector<Glyph>& glyphs)
		{
			UniChar previous;
			
			for(size_t i=0; i<glyphs.size(); i++)
			{
				const Glyph& glyph = glyphs[i];
				
				_extents.x += glyph.AdvanceX();
				
				if(i > 0)
					_extents.x += glyph.Kerning(previous);
				
				previous = glyph.Character();
			}
			
			if(!_glyphs.empty())
				_extents.x += glyphs.at(0).Kerning(_glyphs[_glyphs.size() - 1].Character());
			
			_glyphs.insert(_glyphs.end(), glyphs.begin(), glyphs.end());
		}
		
		void LineSegment::InsertGlyph(const Glyph& glyph)
		{
			_extents.x += glyph.AdvanceX();
			
			if(!_glyphs.empty())
				_extents.x += _glyphs[0].Kerning(glyph.Character());
			
			_glyphs.insert(_glyphs.begin(), glyph);
		}
		
		LineSegment LineSegment::SegmentWithWidth(float width, bool reverse)
		{
			LineSegment segment;
			segment.SetFont(_font);
			
			float filled = 0.0f;
			std::vector<Glyph> glyphs;
			
			if(!reverse)
			{
				for(auto i=_glyphs.begin(); i!=_glyphs.end(); i++)
				{
					float glyphWidth = i->AdvanceX();
					
					if(!glyphs.empty())
						glyphWidth += i->Kerning(glyphs[glyphs.size() - 1].Character());
					
					if(filled + glyphWidth > width)
						break;
					
					filled += glyphWidth;
					glyphs.push_back(*i);
				}
			}
			else
			{
				for(auto i=_glyphs.rbegin(); i!=_glyphs.rend(); i++)
				{
					float glyphWidth = i->AdvanceX();
					
					if(!glyphs.empty())
						glyphWidth += glyphs[glyphs.size() - 1].Kerning(i->Character());
					
					if(filled + glyphWidth > width)
						break;
					
					filled += glyphWidth;
					glyphs.push_back(*i);
				}
				
				std::reverse(glyphs.begin(), glyphs.end());
			}
			
			segment.AddGlyphs(glyphs);
			return segment;
		}

		// ---------------------
		// MARK: -
		// MARK: Lines
		// ---------------------
		
		Line::Line(AttributedString *string, const Range& range) :
			_range(range)
		{
			RN_ASSERT(string, "String mustn't be NULL!");
			_string = string->Retain();
			_dirty  = true;
		}
		
		Line::~Line()
		{
			if(_string)
				_string->Release();
		}
		
		
		float Line::TokenWidthInSegment(const LineSegment& segment)
		{
			Font *font = segment.GlyphFont();
			const Glyph& glyph = font->GlyphForCharacter(_truncationToken);
			
			return glyph.AdvanceX();
		}
		
		void Line::Truncate(float width, TextTruncation truncation, UniChar token)
		{
			_truncated = true;
			_truncationWidth = width;
			_truncationType  = truncation;
			_truncationToken = token;
			
			_dirty = true;
		}
		
		const std::vector<LineSegment>& Line::Segments()
		{
			LayoutLine();
			return _segments;
		}
		
		const Vector2& Line::Extents()
		{
			LayoutLine();
			return _extents;
		}
		
		void Line::LayoutLine()
		{
			if(!_dirty)
				return;
			
			_segments.clear();
			
			String *string = _string->String();
			Font   *font = nullptr;
			
			AutoreleasePool *pool = new AutoreleasePool();
			LineSegment segment;
			
			// Create all segments of the line
			for(size_t i=0; i<_range.length; i++)
			{
				UniChar character = string->CharacterAtIndex(static_cast<uint32>(_range.origin + i));
				Dictionary *attributes = _string->AttributesAtIndex(_range.origin + i);
				
				Font *glyphFont = Typesetter::FontForAttributes(attributes);
				if(glyphFont != font)
				{
					if(font)
					{
						LineSegment temp;
						std::swap(temp, segment);
						
						if(temp.IsValid())
							_segments.push_back(std::move(temp));
					}
					
					font = glyphFont;
					segment.SetFont(font);
				}
				
				const Glyph& glyph = font->GlyphForCharacter(character);
				segment.AddGlyph(glyph);
			}
			
			if(segment.IsValid())
				_segments.push_back(segment);
			
			delete pool;
			UpdateExtents();

			// Apply truncation
			if(_truncated && _extents.x > _truncationWidth)
			{
				std::vector<LineSegment> truncated;
				float truncateWidth = _truncationWidth;
				bool truncateMiddle = false;
				
				if(_truncationType == TextTruncation::Middle)
				{
					truncateWidth  = _truncationWidth * 0.5f;
					truncateMiddle = true;
				}
				
				
				if(_truncationType == TextTruncation::End || _truncationType == TextTruncation::Middle)
				{
					float filled = 0.0f;
					for(auto i=_segments.begin(); i!=_segments.end(); i++)
					{
						if(filled + i->Extents().x >= truncateWidth)
						{
							// This is the segment we need to truncated
							float left;
							
							if(truncateMiddle)
							{
								float tokenWidth = TokenWidthInSegment(*i) * 0.5f;
								
								left = truncateWidth - (filled + tokenWidth);
								truncateWidth -= tokenWidth;
							}
							else
								left = truncateWidth - (filled + TokenWidthInSegment(*i));
							
							LineSegment segment = std::move(i->SegmentWithWidth(left, false));
							segment.AddGlyph(font->GlyphForCharacter(_truncationToken));
							
							truncated.push_back(segment);
							break;
						}
						
						filled += i->Extents().x;
						truncated.push_back(*i);
					}
				}
				
				if(_truncationType == TextTruncation::Start || _truncationType == TextTruncation::Middle)
				{
					float filled = 0.0f;
					for(auto i=_segments.rbegin(); i!=_segments.rend(); i++)
					{
						if(filled + i->Extents().x >= truncateWidth)
						{
							// This is the segment we need to truncated
							float left;
							left = truncateMiddle ? truncateWidth - filled : truncateWidth - (filled + TokenWidthInSegment(*i));
							
							LineSegment segment = std::move(i->SegmentWithWidth(left, true));
							
							if(!truncateMiddle)
								segment.InsertGlyph(font->GlyphForCharacter(_truncationToken));
							
							truncated.push_back(segment);
							break;
						}
						
						filled += i->Extents().x;
						truncated.push_back(*i);
					}
				}
				
				std::swap(_segments, truncated);
				UpdateExtents();
			}
			
			_dirty = false;
		}
		
		void Line::UpdateExtents()
		{
			_extents = Vector2(0.0f);
			
			for(const LineSegment& segment : _segments)
			{
				_extents.x += segment.Extents().x;
				_extents.y = std::max(_extents.y, segment.Extents().y);
			}
		}
	}
}
