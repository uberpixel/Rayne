//
//  RNUIFontManager.cpp
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNUIFontManager.h"
#include <KGMeshGeneratorLoopBlinn.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>
#include <artery-font.h>
#include <stdio-serialization.h>
#include <std-artery-font.h>

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Font, RN::Object)
		RNDefineMeta(FontManager, RN::Object)
	
		namespace internal
		{
			int fileRead(void *buffer, int length, void *file)
			{
				return reinterpret_cast<File *>(file)->Read(buffer, length);
			}

			int fileWrite(const void *buffer, int length, void *file)
			{
				return reinterpret_cast<File *>(file)->Write(buffer, length);
			}
		}

		Font::Font(RN::String *filepath, bool preloadASCII) : _fontInfo(nullptr), _arFont(nullptr), _fontTexture(nullptr)
		{
			Lock();
			_meshes = new RN::Dictionary();
			
			if(filepath->HasSuffix(RNCSTR(".arfont")))
			{
				File *file = File::WithName(filepath);
				auto *arFont = new artery_font::StdArteryFont<float>();
				artery_font::decode<internal::fileRead>(*arFont, file);
				_arFont = arFont;
				auto variant = arFont->variants[0];
				_textureResolution.x = arFont->images[0].width;
				_textureResolution.y = arFont->images[0].height;
				Data *pngData = new Data(&arFont->images[0].data[0], arFont->images[0].data.length(), true, false);
				Dictionary *settings = new Dictionary();
				settings->SetObjectForKey(Number::WithBool(true), RNCSTR("isLinear"));
				settings->SetObjectForKey(Number::WithBool(false), RNCSTR("mipMapped"));
				_fontTexture = Texture::WithPNGData(pngData, settings);
				settings->Release();
				SafeRetain(_fontTexture);
				pngData->Release();
				
				for(int i = 0; i < variant.glyphs.length(); i++)
				{
					_codePointToIndex[variant.glyphs[i].codepoint] = i;
				}
			}
			else
			{
				_fontData = RN::Data::WithContentsOfFile(filepath);
				_fontData->Retain();
				_fontInfo = new stbtt_fontinfo;
				stbtt_InitFont(_fontInfo, _fontData->GetBytes<unsigned char>(), stbtt_GetFontOffsetForIndex(_fontData->GetBytes<unsigned char>(), 0));
				
				if(preloadASCII)
				{
					for(int i = 0; i < 128; i++)
					{
						GetMeshForCharacter(i);
					}
				}
			}
			Unlock();
		}

		Font::~Font()
		{
			Lock();
			if(_arFont)
			{
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				delete arFont;
			}
			if(_fontInfo) delete _fontInfo;
			SafeRelease(_fontData);
			SafeRelease(_meshes);
			SafeRelease(_fontTexture);
			Unlock();
		}

		RN::Mesh *Font::GetMeshForCharacter(int codepoint)
		{
			//Don not generate a mesh for control characters!
			if(codepoint <= 32) return nullptr;
			
			Lock();
			RN::Mesh *existingMesh = _meshes->GetObjectForKey<RN::Mesh>(RN::Number::WithInt32(codepoint));
			if(existingMesh)
			{
				Unlock();
				return existingMesh;
			}
			
			if(_arFont)
			{
				if(_codePointToIndex.count(codepoint) == 0) return nullptr; //There is no character for the requested codepoint
				
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				auto variant = arFont->variants[0];
				
				auto foundGlyph = variant.glyphs[_codePointToIndex[codepoint]];
				
				float *vertexPositionBuffer = new float[4 * 2];
				float *vertexUVBuffer = new float[4 * 4];
				uint32 *indexBuffer = new uint32[6];
				
				vertexPositionBuffer[0 * 2 + 0] = foundGlyph.planeBounds.l;
				vertexPositionBuffer[0 * 2 + 1] = foundGlyph.planeBounds.t;
				
				vertexPositionBuffer[1 * 2 + 0] = foundGlyph.planeBounds.r;
				vertexPositionBuffer[1 * 2 + 1] = foundGlyph.planeBounds.t;
				
				vertexPositionBuffer[2 * 2 + 0] = foundGlyph.planeBounds.r;
				vertexPositionBuffer[2 * 2 + 1] = foundGlyph.planeBounds.b;
				
				vertexPositionBuffer[3 * 2 + 0] = foundGlyph.planeBounds.l;
				vertexPositionBuffer[3 * 2 + 1] = foundGlyph.planeBounds.b;
				
				vertexUVBuffer[0 * 4 + 0] = foundGlyph.imageBounds.l / _textureResolution.x;
				vertexUVBuffer[0 * 4 + 1] = 1.0f-foundGlyph.imageBounds.t / _textureResolution.y;
				
				vertexUVBuffer[1 * 4 + 0] = foundGlyph.imageBounds.r / _textureResolution.x;
				vertexUVBuffer[1 * 4 + 1] = 1.0f-foundGlyph.imageBounds.t / _textureResolution.y;
				
				vertexUVBuffer[2 * 4 + 0] = foundGlyph.imageBounds.r / _textureResolution.x;
				vertexUVBuffer[2 * 4 + 1] = 1.0f-foundGlyph.imageBounds.b / _textureResolution.y;
				
				vertexUVBuffer[3 * 4 + 0] = foundGlyph.imageBounds.l / _textureResolution.x;
				vertexUVBuffer[3 * 4 + 1] = 1.0f-foundGlyph.imageBounds.b / _textureResolution.y;
				
				indexBuffer[0] = 0;
				indexBuffer[1] = 3;
				indexBuffer[2] = 1;
				
				indexBuffer[3] = 3;
				indexBuffer[4] = 2;
				indexBuffer[5] = 1;
				
				std::vector<Mesh::VertexAttribute> meshVertexAttributes;
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector4);
				
				Mesh *mesh = new Mesh(meshVertexAttributes, 4, 6);
				mesh->BeginChanges();
				
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUVBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
				
				mesh->EndChanges();
				
				delete[] vertexPositionBuffer;
				delete[] vertexUVBuffer;
				delete[] indexBuffer;
				
				_meshes->SetObjectForKey(mesh, RN::Number::WithInt32(codepoint));
				Unlock();
				
				return mesh;
			}
			
			stbtt_vertex *shapeVertices;
			int shapeVertexCount = stbtt_GetCodepointShape(_fontInfo, codepoint, &shapeVertices);
			//int shapeVertexCount = stbtt_GetGlyphShape(font, glyphIndex, &shapeVertices);
			
			KG::PathCollection paths;
			KG::Path mypath;
			KG::Vector2 from = { 0.0, 0.0 };
			for(int i = 0; i < shapeVertexCount; i++)
			{
				stbtt_vertex vertex = shapeVertices[i];
				if(vertex.type == STBTT_vmove)
				{
					if(mypath.segments.size() > 0)
					{
						paths.paths.push_back(mypath);
						mypath.segments.clear();
					}
					
					from.x = vertex.x;
					from.y = vertex.y;
				}
				else if(vertex.type == STBTT_vline)
				{
					KG::PathSegment segment;
					
					KG::Vector2 to;
					to.x = vertex.x;
					to.y = vertex.y;
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.push_back(from);
					segment.controlPoints.push_back(to);
					
					mypath.segments.push_back(segment);
					
					from = to;
				}
				else if(vertex.type == STBTT_vcurve)
				{
					KG::PathSegment segment;
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					
					KG::Vector2 to;
					to.x = vertex.x;
					to.y = vertex.y;
					
					KG::Vector2 control;
					control.x = vertex.cx;
					control.y = vertex.cy;
					
					segment.controlPoints.push_back(from);
					segment.controlPoints.push_back(control);
					segment.controlPoints.push_back(to);
					
					mypath.segments.push_back(segment);
					
					from = to;
				}
				else if(vertex.type == STBTT_vcubic)
				{
					KG::PathSegment segment;
					segment.type = KG::PathSegment::TypeBezierCubic;
					
					KG::Vector2 to;
					to.x = vertex.x;
					to.y = vertex.y;
					
					KG::Vector2 control[2];
					control[0].x = vertex.cx;
					control[0].y = vertex.cy;
					control[1].x = vertex.cx1;
					control[1].y = vertex.cy1;
					
					segment.controlPoints.push_back(from);
					segment.controlPoints.push_back(control[0]);
					segment.controlPoints.push_back(control[1]);
					segment.controlPoints.push_back(to);
					
					mypath.segments.push_back(segment);
					
					from = to;
				}
			}
			
			stbtt_FreeShape(_fontInfo, shapeVertices);
			
			if(mypath.segments.size() > 0)
			{
				paths.paths.push_back(mypath);
			}
			
			if(paths.paths.size() == 0)
			{
				Unlock();
				return nullptr;
			}
			
			KG::TriangleMesh triangleMesh = KG::MeshGeneratorLoopBlinn::GetMeshForPathCollection(paths);
			
			std::vector<RN::Mesh::VertexAttribute> attributes;
			attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Indices, RN::PrimitiveType::Uint32);

			RN::int32 vertexFloatCount = 0;
			for(KG::TriangleMesh::VertexFeature feature : triangleMesh.features)
			{
				switch(feature)
				{
					case KG::TriangleMesh::VertexFeaturePosition:
						attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Vertices, RN::PrimitiveType::Vector2);
						vertexFloatCount += 2;
						break;
						
					case KG::TriangleMesh::VertexFeatureUV:
						attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::UVCoords0, RN::PrimitiveType::Vector3);
						vertexFloatCount += 3;
						break;
						
					case KG::TriangleMesh::VertexFeatureColor:
						attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Color0, RN::PrimitiveType::Vector4);
						vertexFloatCount += 4;
						break;
				}
			}

			RN::uint32 verticesCount = triangleMesh.vertices.size() / vertexFloatCount;
			RN::Mesh *mesh = new RN::Mesh(attributes, verticesCount, triangleMesh.indices.size());
			mesh->BeginChanges();

			float *buildBuffer = new float[verticesCount * 4];
			
			#define CopyVertexData(elementSize, offset, feature) \
			{ \
				RN::uint32 dataIndex = 0; \
				RN::uint32 buildBufferIndex = 0; \
				for(size_t i = 0; i < verticesCount; i ++) \
				{ \
					std::copy(triangleMesh.vertices.begin() + dataIndex + offset, triangleMesh.vertices.begin() + dataIndex + offset + elementSize, &buildBuffer[buildBufferIndex]); \
					buildBufferIndex += elementSize; \
					dataIndex += vertexFloatCount; \
				} \
				mesh->SetElementData(feature, buildBuffer); \
			}
			
			RN::uint32 offset = 0;
			for(KG::TriangleMesh::VertexFeature feature : triangleMesh.features)
			{
				switch(feature)
				{
					case KG::TriangleMesh::VertexFeaturePosition:
						CopyVertexData(2, offset, RN::Mesh::VertexAttribute::Feature::Vertices);
						offset += 2;
						break;
						
					case KG::TriangleMesh::VertexFeatureUV:
						CopyVertexData(3, offset, RN::Mesh::VertexAttribute::Feature::UVCoords0);
						offset += 3;
						break;
						
					case KG::TriangleMesh::VertexFeatureColor:
						CopyVertexData(4, offset, RN::Mesh::VertexAttribute::Feature::Color0);
						offset += 4;
						break;
				}
			}

			delete[] buildBuffer;
			mesh->SetElementData(RN::Mesh::VertexAttribute::Feature::Indices, triangleMesh.indices.data());
			mesh->EndChanges();
			
			_meshes->SetObjectForKey(mesh, RN::Number::WithInt32(codepoint));
			Unlock();
			
			return mesh;
		}

		float Font::GetOffsetForNextCharacter(int currentCodepoint, int nextCodepoint)
		{
			Lock();
			if(_arFont)
			{
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				auto variant = arFont->variants[0];
				
				Vector2 kerning;
				for(int i = 0; i < variant.kernPairs.length(); i++)
				{
					if(variant.kernPairs[i].codepoint1 == currentCodepoint && variant.kernPairs[i].codepoint2 == nextCodepoint)
					{
						kerning = Vector2(variant.kernPairs[i].advance.h, variant.kernPairs[i].advance.v);
						break;
					}
				}
				
				auto foundGlyph = variant.glyphs[_codePointToIndex[currentCodepoint]];
				kerning.x += foundGlyph.advance.h;
				kerning.y += foundGlyph.advance.v;
				
				Unlock();
				return kerning.x;
			}
			
			int advance, lsb;
			stbtt_GetCodepointHMetrics(_fontInfo, currentCodepoint, &advance, &lsb);
			float offset = advance;
			if(nextCodepoint >= 0)
				offset += stbtt_GetCodepointKernAdvance(_fontInfo, currentCodepoint, nextCodepoint);
			Unlock();
			
			return offset;
		}

		float Font::GetAscent()
		{
			Lock();
			if(_arFont)
			{
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				auto variant = arFont->variants[0];
				
				float ascent = variant.metrics.ascender;
				
				Unlock();
				return ascent;
			}
			
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return ascent;
		}

		float Font::GetDescent()
		{
			Lock();
			if(_arFont)
			{
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				auto variant = arFont->variants[0];
				
				float descent = variant.metrics.descender;
				
				Unlock();
				return descent;
			}
			
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return descent;
		}

		float Font::GetLineOffset()
		{
			Lock();
			if(_arFont)
			{
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				auto variant = arFont->variants[0];
				
				float linegap = variant.metrics.lineHeight;
				
				Unlock();
				return linegap;
			}
			
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return linegap;
		}

		float Font::GetHeight()
		{
			Lock();
			if(_arFont)
			{
				artery_font::StdArteryFont<float> *arFont = static_cast<artery_font::StdArteryFont<float>*>(_arFont);
				auto variant = arFont->variants[0];
				
				float height = variant.metrics.ascender - variant.metrics.descender;
				
				Unlock();
				return height;
			}
			
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return ascent - descent;
		}
	
		Texture *Font::GetFontTexture()
		{
			return _fontTexture;
		}
	
		bool Font::IsSDFFont()
		{
			return _arFont != nullptr;
		}

		FontManager::FontManager()
		{
			Lock();
			_fonts = new RN::Dictionary();
			Unlock();
		}

		FontManager::~FontManager()
		{
			Lock();
			_fonts->Release();
			Unlock();
		}

		Font *FontManager::GetFontForFilepath(RN::String *filepath, bool preloadASCII)
		{
			Lock();
			Font *font = _fonts->GetObjectForKey<Font>(filepath);
			if(font)
			{
				Unlock();
				return font;
			}
			
			font = new Font(filepath, preloadASCII);
			_fonts->SetObjectForKey(font, filepath);
			Unlock();
			
			return font;
		}
	
		FontManager *FontManager::_sharedInstance = nullptr;

		FontManager *FontManager::GetSharedInstance()
		{
			if(!_sharedInstance)
			{
				_sharedInstance = new FontManager();
			}
			
			return _sharedInstance;
		}
	}
}
