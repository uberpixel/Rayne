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
#include "RNResourceCoordinator.h"
#include "RNUIStyle.h"

#define kRNTypesetterMeshAttribute RNCSTR("kRNTypesetterMeshAttribute")

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
			_ellipsis  = CodePoint::Ellipsis();
			
			_model = nullptr;
			_dirty = true;
			_allowClippedLines = true;
		}
		
		Typesetter::~Typesetter()
		{
			_string->Release();
			
			if(_model)
				_model->Release();
			
			Clear();
		}
		
		Font *Typesetter::FontForAttributes(Dictionary *attributes)
		{
			static Font *defaultFont = nullptr;
			static std::once_flag flag;
			
			std::call_once(flag, [&]() {
				defaultFont = Style::GetSharedInstance()->GetFont(UI::Style::FontStyle::DefaultFont);
			});
			
			Font *font = attributes->GetObjectForKey<Font>(kRNTypesetterFontAttribute);
			return font ? font : defaultFont;
		}
		
		const RN::Color& Typesetter::ColorForAttributes(Dictionary *attributes)
		{
			static RN::Color defaultColor = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
			
			Color *color = attributes->GetObjectForKey<Color>(kRNTypesetterColorAttribute);
			return color ? color->GetRNColor() : defaultColor;
		}
		
		
		
		void Typesetter::SetText(AttributedString *string)
		{
			RN_ASSERT(string, "String mustn't be NULL!");
			
			_string->Release();
			_string = string->Retain();
			_dirty  = true;
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
		
		void Typesetter::SetAllowPartiallyClippedLined(bool allowClippedLines)
		{
			_allowClippedLines = allowClippedLines;
			_frameChanged = true;
		}
		
		void Typesetter::SetFrame(const Rect& frame)
		{
			if(Math::Compare(frame.width, _frame.width) && Math::Compare(frame.height, _frame.height))
			{
				_frameChanged = true;
				_frame = frame;
				
				return;
			}
			
			_frame = frame;
			_dirty = true;
		}
		
		void Typesetter::SetEllipsis(UniChar character)
		{
			_ellipsis = character;
			_dirty    = true;
		}
		
		
		void Typesetter::InvalidateStringInRange(const Range& range)
		{
			_dirty = true;
		}
		
		
		Line *Typesetter::GetLineAtPoint(const Vector2& point)
		{
			LayoutText();
			
			Vector2 dimensions = std::move(GetDimensions());
			if(point.x > dimensions.x || point.y > dimensions.y)
				return nullptr;
			
			for(Line *line : _lines)
			{
				if(line->GetFrame().ContainsPoint(point))
					return line;
			}
			
			return nullptr;
		}
		
		Range Typesetter::GetRangeOfCharactersAtPoint(const Vector2& point)
		{
			LayoutText();
			
			Line *line = GetLineAtPoint(point);
			if(line)
				return line->GetRangeOfCharactersAtPoint(point);
			
			return Range(k::NotFound, 0);
		}
		
		Vector2 Typesetter::GetDimensions()
		{
			LayoutText();
			
			Vector2 extents;
			for(Line *line : _lines)
			{
				extents.x = std::max(line->GetUntruncatedExtents().x, extents.x);
				extents.y += line->GetUntruncatedExtents().y;
			}
			
			return extents;
		}
		
		Vector2 Typesetter::GetVisibleDimensions()
		{
			LayoutText();
			
			Vector2 extents;
			for(Line *line : _visibleLines)
			{
				extents.x = std::max(line->GetExtents().x, extents.x);
				extents.y += line->GetExtents().y;
			}
			
			return extents;
		}
		
		
		Model *Typesetter::GetLineModel()
		{
			LayoutText();
			return _model;
		}
		
		const std::vector<Line *>& Typesetter::GetLines()
		{
			LayoutText();
			return _lines;
		}
		
		const std::vector<Line *>& Typesetter::GetVisibleLines()
		{
			LayoutText();
			CalculateVisibleLines();
			return _visibleLines;
		}
		
		
		
		void Typesetter::Clear()
		{
			for(auto i = _lines.begin(); i != _lines.end(); i ++)
				delete *i;
			
			_lines.clear();
			_visibleLines.clear();
		}
		
		void Typesetter::CalculateVisibleLines()
		{
			if(!_frameChanged)
				return;
			
			_visibleLines.clear();
			
			float offset = 0.0f;
			
			for(Line *line : _lines)
			{
				Rect lineRect(_frame.x, offset, _frame.width, line->GetExtents().y);
				offset += lineRect.height;
				
				if(_allowClippedLines)
				{
					if(_frame.IntersectsRect(lineRect))
						_visibleLines.push_back(line);
				}
				else
				{
					if(_frame.ContainsRect(lineRect))
						_visibleLines.push_back(line);
				}
			}
			
			_frameChanged = false;
		}
		
		void Typesetter::LayoutText()
		{
			if(!_dirty)
				return;
			
			Clear();
			
			size_t length  = _string->GetLength();
			String *string = _string->GetString();
			
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
				line->_typesetter = this; \
				_lines.push_back(line); \
				\
				if(truncateLine) \
					line->Truncate(_frame.width, truncation, _ellipsis); \
			}
			
#define LineBreak(forced) { \
				offsetX = 0.0f; \
				onNewLine = true; \
				forcedLinebreak = forced; \
				lines ++; \
				if(_maxLines > 0 && lines >= _maxLines) \
					goto finishLayout; \
			}
			
#define LineBreakOnCharacter(forced) { \
				SubmitLine(); \
				LineBreak(forced); \
				range.origin += range.length; \
				range.length = 0; \
				i --; \
			}
			
			
			AutoreleasePool *pool = new AutoreleasePool();
			
			float wordWidth = 0.0f;
			Range wordRange = Range(k::NotFound, 0);
			
			for(size_t i = 0; i < length; i++)
			{
				UniChar character = string->GetCharacterAtIndex(i);
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
				
				Font *font = FontForAttributes(_string->GetAttributesAtIndex(i));
				const Glyph& glyph = font->GetGlyphForCharacter(character);
				
				float width = glyph.GetAdvanceX();
				
				if(!onNewLine)
					width += glyph.GetKerning(previous);
				
				forcedLinebreak = false;
				onNewLine = false;
				previous  = character;
				
				switch(_lineBreak)
				{
					case LineBreakMode::CharacterWrapping:
						if(offsetX + width >= _frame.width)
						{
							LineBreakOnCharacter(true);
							continue;
						}
						
						offsetX += width;
						range.length ++;
						break;
						
					case LineBreakMode::None:
					case LineBreakMode::TruncateHead:
					case LineBreakMode::TruncateMiddle:
					case LineBreakMode::TruncateTail:
						offsetX += width;
						range.length ++;
						break;
						
					case LineBreakMode::WordWrapping:
						
						if(point.IsWhitespace() || onNewLine)
							wordRange.origin = k::NotFound;
						
						// Look ahead the next complete word
						if(wordRange.origin == k::NotFound && !point.IsWhitespace() && !point.IsNewline())
						{
							wordRange = Range(i, 0);
							wordWidth = 0.0f;
							
							for(size_t j = i; j < length; j ++)
							{
								CodePoint temp = CodePoint(string->GetCharacterAtIndex(j));
								
								if(temp.IsWhitespace() || temp.IsNewline())
									break;
								
								const Glyph& tglyph = font->GetGlyphForCharacter(character);
								wordWidth += tglyph.GetAdvanceX();
								wordRange.length ++;
							}
							
							if(wordRange.length > 0)
							{
								if(offsetX + wordWidth >= _frame.width)
								{
									LineBreakOnCharacter(false);
									continue;
								}
							}
							else
							{
								wordRange.origin = k::NotFound;
							}
						}
						
						// We still have to break on characters for words wich are longer than one line
						if(offsetX + width >= _frame.width)
						{
							LineBreakOnCharacter(true);
							continue;
						}
						
						offsetX += width;
						range.length ++;
						break;
						
					default:
						throw Exception(Exception::Type::ShaderUnsupportedException, "");
				}
			}
			
			if(range.origin < length && (_maxLines == 0 || lines < _maxLines))
				SubmitLine();
			
		finishLayout:
			delete pool;
			pool = new AutoreleasePool();
			
			float offset = 0.0f;
			
			for(Line *line : _lines)
			{
				const Vector2& extents = line->GetExtents();
				float widthOffset = 0.0f;
				
				switch(_alignment)
				{
					case TextAlignment::Center:
						widthOffset = (_frame.width * 0.5f) - (extents.x * 0.5f);
						break;
						
					case TextAlignment::Right:
						widthOffset = _frame.width - extents.x;
						break;
						
					default:
						break;
				}
				
				line->SetLineOffset(Vector2(widthOffset, offset));
				offset += extents.y;
			}
			
			_dirty = false;
			_frameChanged = true;
			
			CalculateVisibleLines();
			MergeMeshes();
			
			delete pool;
		}
		
		Mesh *Typesetter::DequeMesh(size_t vertices)
		{
			Mesh *closest = nullptr;
			ptrdiff_t diff = 0;
			
			_meshes.Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool *stop) {
				if(!closest)
				{
					closest = mesh;
					diff    = mesh->GetVerticesCount() - vertices;
					
					if(diff == 0)
						*stop = true;
					
					return;
				}
				
				intptr_t difference = mesh->GetVerticesCount() - vertices;
				
				if(std::labs(difference) < std::labs(diff))
				{
					closest = mesh;
					diff    = difference;
					
					if(diff == 0)
						*stop = true;
					
					return;
				}
			});
			
			if(closest)
			{
				closest->Retain()->Autorelease();
				_meshes.RemoveObject(closest);
			}
			
			return closest;
		}
		
		void Typesetter::MergeMeshes()
		{
			Array *meshes = new Array();
			
			for(Line *line : _visibleLines)
				meshes->AddObjectsFromArray(line->GetMeshes(_frame.height));
			
			
			if(_model)
				_model->Release();
			
			_model = new Model();
			
			
			Shader *shader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyUITextShader, nullptr);
			
			meshes->Enumerate([&](Object *object, size_t index, bool *stop) {
				
				Dictionary *dict = static_cast<Dictionary *>(object);
				
				Mesh *mesh = dict->GetObjectForKey<Mesh>(kRNTypesetterMeshAttribute);
				Font *font = dict->GetObjectForKey<Font>(kRNTypesetterFontAttribute);
				Color *color = dict->GetObjectForKey<Color>(kRNTypesetterColorAttribute);
				
				_meshes.AddObject(mesh);
				
				RN_ASSERT(mesh && font && color, "Inconsistent mesh data generated!");
				
				Material *material = new Material(shader);
				material->AddTexture(font->GetTexture());
				material->depthtest = false;
				material->depthwrite = false;
				material->blending = true;
				material->lighting = false;
				material->ambient  = color->GetRNColor();
				
				if(font->GetFiltering())
					material->Define("RN_SUBPIXEL_ANTIALIAS");
				
				_model->AddMesh(mesh, material->Autorelease(), 0);
			});
			
			meshes->Release();
		}
		
		// ---------------------
		// MARK: -
		// MARK: LineSegments
		// ---------------------
		
		LineSegment::LineSegment()
		{
			_font = nullptr;
			_range = Range(0, 0);
		}
		LineSegment::LineSegment(const LineSegment& other)
		{
			_font    = other._font ? other._font->Retain() : nullptr;
			_glyphs  = other._glyphs;
			_extents = other._extents;
			_color   = other._color;
			_range   = other._range;
		}
		LineSegment::LineSegment(LineSegment&& other)
		{
			_font    = other._font;
			_glyphs  = std::move(other._glyphs);
			_extents = std::move(other._extents);
			_color   = std::move(other._color);
			_range   = std::move(other._range);
			
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
			_color   = other._color;
			_range   = other._range;
			
			return *this;
		}
		LineSegment& LineSegment::operator= (LineSegment&& other)
		{
			_font    = other._font;
			_glyphs  = std::move(other._glyphs);
			_extents = std::move(other._extents);
			_color   = std::move(other._color);
			_range   = std::move(other._range);
			
			other._font = nullptr;
			return *this;
		}
		
		void LineSegment::SetFont(Font *font)
		{
			if(_font)
				_font->Release();
			
			_font = font->Retain();
			_extents.y = _font->GetDefaultLineHeight();
		}
		
		void LineSegment::SetColor(const RN::Color& color)
		{
			_color = color;
		}
		
		void LineSegment::SetOffset(const Vector2& offset)
		{
			_offset = offset;
		}
		
		void LineSegment::SetRangeOffset(size_t offset)
		{
			_range.origin = offset;
		}
		
		bool LineSegment::CanMergeWithSegment(const LineSegment& other)
		{
			return (_font == other._font && _color == other._color);
		}
		
		bool LineSegment::IsValidGlyph(const Glyph& glyph) const
		{
			CodePoint point(glyph.GetCharacter());
			
			if(point.IsNewline())
				return false;
			
			return true;
		}
		
		Range LineSegment::GetRangeOfCharactersAtPoint(const Vector2& point) const
		{
			float offset = 0;
			Range range = Range(k::NotFound, 0);
			
			for(size_t i = 0; i < _range.length; i ++)
			{
				const Glyph& glyph = _glyphs[i];
				float width = glyph.GetAdvanceX();
				
				if(i > 0)
					width += glyph.GetKerning(_glyphs[i - 1].GetCharacter());
				
				Rect frame = Rect(_offset.x + offset, _offset.y, width, _extents.y);
				
				if(frame.ContainsPoint(point))
				{
					if(range.origin == k::NotFound)
						range.origin = _range.origin + i;
					
					range.length ++;
				}
				else if(range.origin != k::NotFound)
				{
					break;
				}
				
				offset += width;
			}
			
			return range;
		}
		
		void LineSegment::AppendGlyph(const Glyph& glyph)
		{
			if(!IsValidGlyph(glyph))
				return;
			
			_glyphs.push_back(glyph);
			_extents.x += glyph.GetAdvanceX();
			
			if(_glyphs.size() > 1)
				_extents.x += glyph.GetKerning(_glyphs[_glyphs.size() - 2].GetCharacter());
			
			_range.length ++;
		}
		
		void LineSegment::AppendGlyphs(const std::vector<Glyph>& glyphs)
		{
			UniChar previous;
			
			for(size_t i = 0; i < glyphs.size(); i ++)
			{
				const Glyph& glyph = glyphs[i];
				if(!IsValidGlyph(glyph))
					continue;
				
				_extents.x += glyph.GetAdvanceX();
				
				if(i > 0)
					_extents.x += glyph.GetKerning(previous);
				
				previous = glyph.GetCharacter();
			}
			
			if(!_glyphs.empty())
				_extents.x += glyphs.at(0).GetKerning(_glyphs[_glyphs.size() - 1].GetCharacter());
			
			_glyphs.insert(_glyphs.end(), glyphs.begin(), glyphs.end());
			_range.length += glyphs.size();
		}
		
		void LineSegment::PrependGlyph(const Glyph& glyph)
		{
			if(!IsValidGlyph(glyph))
				return;
			
			_extents.x += glyph.GetAdvanceX();
			
			if(!_glyphs.empty())
				_extents.x += _glyphs[0].GetKerning(glyph.GetCharacter());
			
			_glyphs.insert(_glyphs.begin(), glyph);
			_range.length ++;
		}
		
		
		LineSegment LineSegment::SegmentWithWidth(float width, bool reverse)
		{
			LineSegment segment;
			segment.SetFont(_font);
			segment.SetColor(_color);
			
			float filled = 0.0f;
			std::vector<Glyph> glyphs;
			
			size_t start = 0;
			
			if(!reverse)
			{
				for(auto i = _glyphs.begin(); i != _glyphs.end(); i ++)
				{
					float glyphWidth = i->GetAdvanceX();
					
					if(!glyphs.empty())
						glyphWidth += i->GetKerning(glyphs[glyphs.size() - 1].GetCharacter());
					
					if(filled + glyphWidth > width)
						break;
					
					filled += glyphWidth;
					glyphs.push_back(*i);
				}
			}
			else
			{
				start = _glyphs.size() - 1;
				
				for(auto i = _glyphs.rbegin(); i != _glyphs.rend(); i ++)
				{
					float glyphWidth = i->GetAdvanceX();
					
					if(!glyphs.empty())
						glyphWidth += glyphs[glyphs.size() - 1].GetKerning(i->GetCharacter());
					
					if(filled + glyphWidth > width)
						break;
					
					filled += glyphWidth;
					start  --;
					
					glyphs.push_back(*i);
				}
				
				std::reverse(glyphs.begin(), glyphs.end());
			}
			
			segment.AppendGlyphs(glyphs);
			segment.SetRangeOffset(start);
			
			return segment;
		}
		
		void LineSegment::CreateGlyphMesh(Vector2 *vertices, Vector2 *uvCoords, uint16 *indices, size_t offset)
		{
			float offsetX = _offset.x;
			float offsetY = -_offset.y;
			
			UniChar previous;
			
			for(size_t i = 0; i < _glyphs.size(); i ++)
			{				
				Glyph& glyph = _glyphs[i];
				
				float x0 = offsetX + glyph.GetOffsetX();
				float y1 = offsetY + glyph.GetOffsetY() - _extents.y;
				float x1 = x0 + glyph.GetWidth();
				float y0 = y1 - glyph.GetHeight();
				
				if(i > 0)
				{
					float kerning = glyph.GetKerning(previous);
					
					offsetX += kerning;
					x0 += kerning;
				}
				
				*vertices ++ = Vector2(x1, y1);
				*vertices ++ = Vector2(x0, y1);
				*vertices ++ = Vector2(x1, y0);
				*vertices ++ = Vector2(x0, y0);
				
				*uvCoords ++ = Vector2(glyph.GetU1(), glyph.GetV0());
				*uvCoords ++ = Vector2(glyph.GetU0(), glyph.GetV0());
				*uvCoords ++ = Vector2(glyph.GetU1(), glyph.GetV1());
				*uvCoords ++ = Vector2(glyph.GetU0(), glyph.GetV1());
				
				*indices ++ = ((i + offset) * 4) + 0;
				*indices ++ = ((i + offset) * 4) + 1;
				*indices ++ = ((i + offset) * 4) + 2;
				
				*indices ++ = ((i + offset) * 4) + 1;
				*indices ++ = ((i + offset) * 4) + 3;
				*indices ++ = ((i + offset) * 4) + 2;
				
				offsetX += glyph.GetAdvanceX();
				previous = glyph.GetCharacter();
			}
		}

		// ---------------------
		// MARK: -
		// MARK: Lines
		// ---------------------
		
		Line::Line(AttributedString *string, const Range& range) :
			_range(range),
			_typesetter(nullptr)
		{
			RN_ASSERT(string, "String mustn't be NULL!");
			
			_string    = string->Retain();
			_dirty     = true;
			_truncated = false;
			_offset    = 0.0f;
		}
		
		Line::~Line()
		{
			if(_string)
				_string->Release();
		}
		
		
		float Line::TokenWidthInSegment(const LineSegment& segment)
		{
			Font *font = segment.GetFont();
			const Glyph& glyph = font->GetGlyphForCharacter(_truncationToken);
			
			return glyph.GetAdvanceX();
		}
		
		void Line::Truncate(float width, TextTruncation truncation, UniChar token)
		{
			_truncated = true;
			_truncationWidth = width;
			_truncationType  = truncation;
			_truncationToken = token;
			
			_dirty = true;
		}
		
		void Line::SetLineOffset(const Vector2& offset)
		{
			_offset = offset;
			
			float temp = 0.0f;
			
			for(LineSegment& segment : _segments)
			{
				segment.SetOffset(Vector2(temp + _offset.x, _offset.y));
				temp += segment.GetExtents().x;
			}
		}
		
		const std::vector<LineSegment>& Line::GetSegments()
		{
			LayoutLine();
			return _segments;
		}
		
		const Vector2& Line::GetExtents()
		{
			LayoutLine();
			return _extents;
		}
		
		const Vector2& Line::GetUntruncatedExtents()
		{
			LayoutLine();
			return _untruncatedExtents;
		}
		
		Range Line::GetRangeOfCharactersAtPoint(const Vector2& point)
		{
			LayoutLine();
			
			for(const LineSegment& segment : _segments)
			{
				if(segment.GetFrame().ContainsPoint(point))
					return segment.GetRangeOfCharactersAtPoint(point);
			}
			
			return Range(k::NotFound, 0);
		}
		
		Rect Line::GetFrame()
		{
			LayoutLine();
			return Rect(_offset, _extents);
		}
		
		Array *Line::GetMeshes(float offset)
		{
			LayoutLine();
			return GenerateMeshes(offset);
		}
		
		void Line::LayoutLine()
		{
			if(!_dirty)
				return;
			
			String *string = _string->GetString();
			Font   *font = nullptr;
			RN::Color color;
			
			AutoreleasePool *pool = new AutoreleasePool();
			LineSegment segment;
			
			_segments.clear();
			
			// Create all segments of the line
			for(size_t i = 0; i < _range.length; i++)
			{
				UniChar character = string->GetCharacterAtIndex(static_cast<uint32>(_range.origin + i));
				Dictionary *attributes = _string->GetAttributesAtIndex(_range.origin + i);
				
				Font *glyphFont = Typesetter::FontForAttributes(attributes);
				const RN::Color& glyphColor = Typesetter::ColorForAttributes(attributes);
				
				if(glyphFont != font || color != glyphColor)
				{
					if(segment.IsValid())
					{
						LineSegment temp;
						std::swap(temp, segment);
						
						_segments.push_back(std::move(temp));
					}
					
					
					font = glyphFont;
					color = glyphColor;
					
					segment.SetFont(font);
					segment.SetColor(color);
					segment.SetRangeOffset(_range.origin + i);
				}
				
				const Glyph& glyph = font->GetGlyphForCharacter(character);
				segment.AppendGlyph(glyph);
			}
			
			if(segment.IsValid())
				_segments.push_back(segment);
			
			delete pool;
			UpdateExtents();
			
			_untruncatedExtents = _extents;

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
				
				// Truncate the left
				if(_truncationType == TextTruncation::End || _truncationType == TextTruncation::Middle)
				{
					float filled = 0.0f;
					for(auto i=_segments.begin(); i!=_segments.end(); i++)
					{
						if(filled + i->GetExtents().x >= truncateWidth)
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
							segment.AppendGlyph(segment.GetFont()->GetGlyphForCharacter(_truncationToken));
							
							truncated.push_back(segment);
							break;
						}
						
						filled += i->GetExtents().x;
						truncated.push_back(*i);
					}
				}
				
				// Truncate the right
				if(_truncationType == TextTruncation::Start || _truncationType == TextTruncation::Middle)
				{
					float filled = 0.0f;
					for(auto i=_segments.rbegin(); i!=_segments.rend(); i++)
					{
						if(filled + i->GetExtents().x >= truncateWidth)
						{
							// This is the segment we need to truncated
							float left;
							left = truncateMiddle ? truncateWidth - filled : truncateWidth - (filled + TokenWidthInSegment(*i));
							
							LineSegment segment = std::move(i->SegmentWithWidth(left, true));
							
							if(!truncateMiddle)
								segment.PrependGlyph(segment.GetFont()->GetGlyphForCharacter(_truncationToken));
							
							truncated.push_back(segment);
							break;
						}
						
						filled += i->GetExtents().x;
						truncated.push_back(*i);
					}
				}
				
				std::swap(_segments, truncated);
				UpdateExtents();
			}
			
			SetLineOffset(_offset);
			_dirty = false;
		}
		
		void Line::UpdateExtents()
		{
			_extents = Vector2(0.0f);
			
			for(const LineSegment& segment : _segments)
			{
				_extents.x += segment.GetExtents().x;
				_extents.y = std::max(_extents.y, segment.GetExtents().y);
			}
		}
		
		Dictionary *Line::GenerateMesh(const std::vector<LineSegment *>& segments, float lineOffset)
		{
			size_t glyphCount = 0;
			size_t offset = 0;
			
			for(LineSegment *segment : segments)
				glyphCount += segment->GetGlyphs().size();
			
			
			Mesh *mesh = _typesetter ? _typesetter->DequeMesh(glyphCount * 4) : nullptr;
			
			if(!mesh)
			{
				MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
				vertexDescriptor.elementMember = 2;
				vertexDescriptor.elementSize   = sizeof(Vector2);
				
				MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
				uvDescriptor.elementMember = 2;
				uvDescriptor.elementSize   = sizeof(Vector2);
				
				MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
				indicesDescriptor.elementMember = 1;
				indicesDescriptor.elementSize   = sizeof(uint16);
				
				std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor, indicesDescriptor };
				mesh = new Mesh(descriptors, glyphCount * 4, glyphCount * 6);
				mesh->Autorelease();
			}
			else
			{
				mesh->SetVerticesCount(glyphCount * 4);
				mesh->SetIndicesCount(glyphCount * 6);
			}
			
			Mesh::Chunk chunk  = mesh->GetChunk();
			Mesh::Chunk ichunk = mesh->GetIndicesChunk();
			
			Vector2 *vertices = new Vector2[glyphCount * 4];
			Vector2 *uvCoords = new Vector2[glyphCount * 4];
			uint16  *indices  = new uint16[glyphCount * 6];
			
			for(LineSegment *segment : segments)
			{
				Vector2 *sVertices = vertices + (offset * 4);
				Vector2 *sUVCoords = uvCoords + (offset * 4);
				uint16 *sIndices   = indices  + (offset * 6);
				
				segment->CreateGlyphMesh(sVertices, sUVCoords, sIndices, offset);
				offset += segment->GetGlyphs().size();
			}
			
			Vector2 *tvertices = vertices;
			
			for(size_t i = 0; i < glyphCount * 4; i ++)
			{
				tvertices->y = tvertices->y + lineOffset;
				tvertices ++;
			}
			
			
			chunk.SetData(vertices, kMeshFeatureVertices);
			chunk.SetData(uvCoords, kMeshFeatureUVSet0);
			chunk.CommitChanges();
			
			ichunk.SetData(indices);
			ichunk.CommitChanges();
			
			delete [] vertices;
			delete [] uvCoords;
			delete [] indices;
			
			Dictionary *dictionary = new Dictionary();
			dictionary->SetObjectForKey(Color::WithRNColor(segments[0]->GetColor()), kRNTypesetterColorAttribute);
			dictionary->SetObjectForKey(segments[0]->GetFont(), kRNTypesetterFontAttribute);
			dictionary->SetObjectForKey(mesh, kRNTypesetterMeshAttribute);
			
			return dictionary->Autorelease();
		}
		
		Array *Line::GenerateMeshes(float offset)
		{
			Array *meshes = new Array();
			
			std::vector<std::vector<LineSegment *>> segments;

			for(const LineSegment& segment : _segments)
			{
				LineSegment *psegment = const_cast<LineSegment *>(&segment);
				bool pushed = false;
				
				for(auto i = segments.begin(); i != segments.end(); i ++)
				{
					LineSegment *tsegment = (*i)[0];
					if(tsegment->CanMergeWithSegment(segment))
					{
						i->push_back(psegment);
						
						pushed = true;
						break;
					}
				}
				
				if(!pushed)
				{
					segments.push_back({ psegment });
				}
			}
			
			for(auto i = segments.begin(); i != segments.end(); i ++)
			{
				Dictionary *mesh = GenerateMesh(*i, offset);
				meshes->AddObject(mesh);
			}
			
			return meshes->Autorelease();
		}
	}
}
