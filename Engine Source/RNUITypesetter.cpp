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
		
		void Typesetter::SetAllowPartiallyClippedLined(bool allowClippedLines)
		{
			_allowClippedLines = allowClippedLines;
			_frameChanged = true;
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
		
		
		void Typesetter::InvalidateStringInRange(const Range& range)
		{
			_dirty = true;
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
			for(auto i=_lines.begin(); i!=_lines.end(); i++)
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
				UniChar character = string->GetCharacterAtIndex(static_cast<uint32>(i));
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
						
					case LineBreakMode::None:
					case LineBreakMode::TruncateHead:
					case LineBreakMode::TruncateMiddle:
					case LineBreakMode::TruncateTail:
						offsetX += width;
						range.length ++;
						break;
						
					default:
						throw Exception(Exception::Type::ShaderUnsupportedException, "");
				}
			}
			
			if(range.origin < length && (_maxLines == 0 || lines < _maxLines))
			{
				SubmitLine();
			}
			
		finishLayout:
			delete pool;
			pool = new AutoreleasePool();
			
			float offset = 0.0f;
			
			for(auto i=_lines.begin(); i!=_lines.end(); i++)
			{
				Line *line = *i;
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
				
				line->SetLineOffset(Vector2(widthOffset, -_frame.height + offset));
				
				offset += extents.y;
			}
			
			_dirty = false;
			_frameChanged = true;
			
			CalculateVisibleLines();
			MergeMeshes();
			
			delete pool;
		}
		
		void Typesetter::MergeMeshes()
		{
			Array *meshes = new Array();
			
			for(Line *line : _visibleLines)
			{
				meshes->AddObjectsFromArray(line->GetMeshes());
			}
			
			
			if(_model)
				_model->Release();
			
			_model = new Model();
			
			Shader *shader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyUITextShader);
			
			meshes->Enumerate([&](Object *object, size_t index, bool *stop) {
				
				Dictionary *dict = static_cast<Dictionary *>(object);
				
				Mesh *mesh = dict->GetObjectForKey<Mesh>(kRNTypesetterMeshAttribute);
				Font *font = dict->GetObjectForKey<Font>(kRNTypesetterFontAttribute);
				Color *color = dict->GetObjectForKey<Color>(kRNTypesetterColorAttribute);
				
				RN_ASSERT(mesh && font && color, "Inconsistent mesh data generated!\n");
				
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
		}
		LineSegment::LineSegment(const LineSegment& other)
		{
			_font    = other._font ? other._font->Retain() : nullptr;
			_glyphs  = other._glyphs;
			_extents = other._extents;
			_color   = other._color;
		}
		LineSegment::LineSegment(LineSegment&& other)
		{
			_font    = other._font;
			_glyphs  = std::move(other._glyphs);
			_extents = std::move(other._extents);
			_color   = std::move(other._color);
			
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
			
			return *this;
		}
		LineSegment& LineSegment::operator= (LineSegment&& other)
		{
			_font    = other._font;
			_glyphs  = std::move(other._glyphs);
			_extents = std::move(other._extents);
			_color   = std::move(other._color);
			
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
		
		void LineSegment::AddGlyph(const Glyph& glyph)
		{
			if(!IsValidGlyph(glyph))
				return;
			
			_glyphs.push_back(glyph);
			_extents.x += glyph.GetAdvanceX();
			
			if(_glyphs.size() > 1)
				_extents.x += glyph.GetKerning(_glyphs[_glyphs.size() - 2].GetCharacter());
		}
		
		void LineSegment::AddGlyphs(const std::vector<Glyph>& glyphs)
		{
			UniChar previous;
			
			for(size_t i=0; i<glyphs.size(); i++)
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
		}
		
		void LineSegment::InsertGlyph(const Glyph& glyph)
		{
			if(!IsValidGlyph(glyph))
				return;
			
			_extents.x += glyph.GetAdvanceX();
			
			if(!_glyphs.empty())
				_extents.x += _glyphs[0].GetKerning(glyph.GetCharacter());
			
			_glyphs.insert(_glyphs.begin(), glyph);
		}
		
		LineSegment LineSegment::SegmentWithWidth(float width, bool reverse)
		{
			LineSegment segment;
			segment.SetFont(_font);
			segment.SetColor(_color);
			
			float filled = 0.0f;
			std::vector<Glyph> glyphs;
			
			if(!reverse)
			{
				for(auto i=_glyphs.begin(); i!=_glyphs.end(); i++)
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
				for(auto i=_glyphs.rbegin(); i!=_glyphs.rend(); i++)
				{
					float glyphWidth = i->GetAdvanceX();
					
					if(!glyphs.empty())
						glyphWidth += glyphs[glyphs.size() - 1].GetKerning(i->GetCharacter());
					
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
		
		void LineSegment::CreateGlyphMesh(Vector2 *vertices, Vector2 *uvCoords, uint16 *indices, size_t offset)
		{
			float offsetX = _offset.x;
			float offsetY = -_offset.y;
			
			UniChar previous;
			
			for(size_t i=0; i<_glyphs.size(); i++)
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
				
				x0 = roundf(x0);
				x1 = roundf(x1);
				y0 = roundf(y0);
				y1 = roundf(y1);
				
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
			_range(range)
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
		
		Array *Line::GetMeshes()
		{
			LayoutLine();
			return GenerateMeshes();
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
			for(size_t i=0; i<_range.length; i++)
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
				}
				
				const Glyph& glyph = font->GetGlyphForCharacter(character);
				segment.AddGlyph(glyph);
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
							segment.AddGlyph(segment.GetFont()->GetGlyphForCharacter(_truncationToken));
							
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
								segment.InsertGlyph(segment.GetFont()->GetGlyphForCharacter(_truncationToken));
							
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
		
		Dictionary *Line::GenerateMesh(const std::vector<LineSegment *>& segments)
		{
			size_t glyphCount = 0;
			size_t offset = 0;
			
			for(LineSegment *segment : segments)
				glyphCount += segment->GetGlyphs().size();
			
			
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			vertexDescriptor.elementCount  = glyphCount * 4;
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			uvDescriptor.elementCount  = glyphCount * 4;
			
			MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
			indicesDescriptor.elementMember = 1;
			indicesDescriptor.elementSize   = sizeof(uint16);
			indicesDescriptor.elementCount  = glyphCount * 6;
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor, indicesDescriptor };
			Mesh *mesh = new Mesh(descriptors);
			
			Vector2 *vertices = mesh->GetElement<Vector2>(kMeshFeatureVertices);
			Vector2 *uvCoords = mesh->GetElement<Vector2>(kMeshFeatureUVSet0);
			uint16 *indices   = mesh->GetElement<uint16>(kMeshFeatureIndices);
			
			for(LineSegment *segment : segments)
			{
				Vector2 *sVertices = vertices + (offset * 4);
				Vector2 *sUVCoords = uvCoords + (offset * 4);
				uint16 *sIndices   = indices  + (offset * 6);
				
				segment->CreateGlyphMesh(sVertices, sUVCoords, sIndices, offset);
				offset += segment->GetGlyphs().size();
			}
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->ReleaseElement(kMeshFeatureUVSet0);
			mesh->ReleaseElement(kMeshFeatureIndices);
			
			
			Dictionary *dictionary = new Dictionary();
			dictionary->SetObjectForKey(Color::WithRNColor(segments[0]->GetColor()), kRNTypesetterColorAttribute);
			dictionary->SetObjectForKey(segments[0]->GetFont(), kRNTypesetterFontAttribute);
			dictionary->SetObjectForKey(mesh->Autorelease(), kRNTypesetterMeshAttribute);
			
			return dictionary->Autorelease();
		}
		
		Array *Line::GenerateMeshes()
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
				Dictionary *mesh = GenerateMesh(*i);
				meshes->AddObject(mesh);
			}
			
			return meshes->Autorelease();
		}
	}
}
