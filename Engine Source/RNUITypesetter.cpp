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
				defaultFont = ResourcePool::GetSharedInstance()->GetResourceWithName<Font>(kRNResourceKeyDefaultFont);
			});
			
			Font *font = attributes->GetObjectForKey<Font>(kRNTypesetterFontAttribute);
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
		
		
		Vector2 Typesetter::Dimensions()
		{
			LayoutText();
			
			Vector2 extents;
			for(Line *line : _lines)
			{
				extents.x = std::max(line->UntruncatedExtents().x, extents.x);
				extents.y += line->UntruncatedExtents().y;
			}
			
			return extents;
		}
		
		Vector2 Typesetter::VisibleDimensions()
		{
			LayoutText();
			
			Vector2 extents;
			for(Line *line : _visibleLines)
			{
				extents.x = std::max(line->Extents().x, extents.x);
				extents.y += line->Extents().y;
			}
			
			return extents;
		}
		
		
		Model *Typesetter::LineModel()
		{
			LayoutText();
			return _model;
		}
		
		const std::vector<Line *>& Typesetter::Lines()
		{
			LayoutText();
			return _lines;
		}
		
		const std::vector<Line *>& Typesetter::VisibleLines()
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
				Rect lineRect(_frame.x, offset, _frame.width, line->Extents().y);
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
				const Vector2& extents = line->Extents();
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
			Dictionary *meshes = new Dictionary();
			
			for(Line *line : _visibleLines)
			{
				Dictionary *mergeDict = line->Meshes();
				
				mergeDict->Enumerate([&](Object *object, Object *key, bool *stop) {
					Array *mergees  = static_cast<Array *>(object);
					Mesh *container = meshes->GetObjectForKey<Mesh>(key);
					
					bool skipFirstIndex = false;
					
					if(!container)
					{
						container = mergees->GetObjectAtIndex<Mesh>(0);
						skipFirstIndex = true;
						
						meshes->SetObjectForKey(container, key);
					}
					
					mergees->Enumerate([&](Object *object, size_t index, bool *stop) {
						if(skipFirstIndex && index == 0)
							return;
						
						container->MergeMesh(static_cast<Mesh *>(object));
					});
				});
			}
			
			
			if(_model)
				_model->Release();
			
			_model = new Model();
			
			Shader *shader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyUITextShader);
			
			meshes->Enumerate([&](Object *object, Object *key, bool *stop) {
				Mesh *mesh = static_cast<Mesh *>(object);
				Font *font = static_cast<Font *>(key);
				
				Material *material = new Material(shader);
				material->AddTexture(font->Texture());
				material->depthtest = false;
				material->depthwrite = false;
				material->blending = true;
				material->lighting = false;
				material->ambient = Color::White();
				
				if(font->Filtering())
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
		
		void LineSegment::SetOffset(const Vector2& offset)
		{
			_offset = offset;
		}
		
		bool LineSegment::IsValidGlyph(const Glyph& glyph) const
		{
			CodePoint point(glyph.Character());
			
			if(point.IsNewline())
				return false;
			
			return true;
		}
		
		void LineSegment::AddGlyph(const Glyph& glyph)
		{
			if(!IsValidGlyph(glyph))
				return;
			
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
				if(!IsValidGlyph(glyph))
					continue;
				
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
			if(!IsValidGlyph(glyph))
				return;
			
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
		
		void LineSegment::CreateGlyphMesh(Vector2 *vertices, Vector2 *uvCoords, uint16 *indices, size_t offset)
		{
			float offsetX = _offset.x;
			float offsetY = -_offset.y;
			
			UniChar previous;
			
			for(size_t i=0; i<_glyphs.size(); i++)
			{				
				Glyph& glyph = _glyphs[i];
				
				float x0 = offsetX + glyph.OffsetX();
				float y1 = offsetY + glyph.OffsetY() - _extents.y;
				float x1 = x0 + glyph.Width();
				float y0 = y1 - glyph.Height();
				
				if(i > 0)
				{
					float kerning = glyph.Kerning(previous);
					
					offsetX += kerning;
					x0 += kerning;
				}
				
				*vertices ++ = Vector2(x1, y1);
				*vertices ++ = Vector2(x0, y1);
				*vertices ++ = Vector2(x1, y0);
				*vertices ++ = Vector2(x0, y0);
				
				*uvCoords ++ = Vector2(glyph.U1(), glyph.V0());
				*uvCoords ++ = Vector2(glyph.U0(), glyph.V0());
				*uvCoords ++ = Vector2(glyph.U1(), glyph.V1());
				*uvCoords ++ = Vector2(glyph.U0(), glyph.V1());
				
				*indices ++ = ((i + offset) * 4) + 0;
				*indices ++ = ((i + offset) * 4) + 1;
				*indices ++ = ((i + offset) * 4) + 2;
				
				*indices ++ = ((i + offset) * 4) + 1;
				*indices ++ = ((i + offset) * 4) + 3;
				*indices ++ = ((i + offset) * 4) + 2;
				
				offsetX += glyph.AdvanceX();
				previous = glyph.Character();
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
			
			_string = string->Retain();
			_dirty  = true;
			_offset = 0.0f;
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
		
		void Line::SetLineOffset(const Vector2& offset)
		{
			_offset = offset;
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
		
		const Vector2& Line::UntruncatedExtents()
		{
			LayoutLine();
			return _untruncatedExtents;
		}
		
		Dictionary *Line::Meshes()
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
			
			AutoreleasePool *pool = new AutoreleasePool();
			LineSegment segment;
			
			_segments.clear();
			
			// Create all segments of the line
			for(size_t i=0; i<_range.length; i++)
			{
				UniChar character = string->GetCharacterAtIndex(static_cast<uint32>(_range.origin + i));
				Dictionary *attributes = _string->GetAttributesAtIndex(_range.origin + i);
				
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
							segment.AddGlyph(segment.GlyphFont()->GlyphForCharacter(_truncationToken));
							
							truncated.push_back(segment);
							break;
						}
						
						filled += i->Extents().x;
						truncated.push_back(*i);
					}
				}
				
				// Truncate the right
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
								segment.InsertGlyph(segment.GlyphFont()->GlyphForCharacter(_truncationToken));
							
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
		
		void Line::GenerateMesh(Dictionary *dictionary, Font *font, const std::vector<LineSegment *>& segments)
		{
			size_t glyphCount = 0;
			size_t offset = 0;
			
			for(LineSegment *segment : segments)
				glyphCount += segment->Glyphs().size();
			
			
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
				offset += segment->Glyphs().size();
			}
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->ReleaseElement(kMeshFeatureUVSet0);
			mesh->ReleaseElement(kMeshFeatureIndices);
			
			Array *meshes = dictionary->GetObjectForKey<Array>(font);
			if(!meshes)
			{
				meshes = new Array();
				meshes->Autorelease();
				
				dictionary->SetObjectForKey(meshes, font);
			}
			
			meshes->AddObject(mesh->Autorelease());
		}
		
		Dictionary *Line::GenerateMeshes()
		{
			Dictionary *meshes = new Dictionary();
			
			float offset = 0.0f;
			std::unordered_map<Font *, std::vector<LineSegment *>> segments;
			
			for(const LineSegment& segment : _segments)
			{
				LineSegment *psegment = const_cast<LineSegment *>(&segment);
				Font *font = psegment->GlyphFont();
				
				auto iterator = segments.find(font);
				
				if(iterator == segments.end())
				{
					std::vector<LineSegment *> tsegments = { psegment };
					segments.insert(std::unordered_map<Font *, std::vector<LineSegment *>>::value_type(font, std::move(tsegments)));
				}
				else
					iterator->second.push_back(psegment);
				
				psegment->SetOffset(Vector2(offset + _offset.x, _offset.y));
				offset += psegment->Extents().x;
			}
			
			for(auto i=segments.begin(); i!=segments.end(); i++)
				GenerateMesh(meshes, i->first, i->second);
			
			return meshes->Autorelease();
		}
	}
}
