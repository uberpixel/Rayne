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

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Font, RN::Object)
		RNDefineMeta(FontManager, RN::Object)

		Font::Font(RN::String *filepath, bool preloadASCII)
		{
			Lock();
			_fontData = RN::Data::WithContentsOfFile(filepath);
			_fontData->Retain();
			_fontInfo = new stbtt_fontinfo;
			stbtt_InitFont(_fontInfo, _fontData->GetBytes<unsigned char>(), stbtt_GetFontOffsetForIndex(_fontData->GetBytes<unsigned char>(), 0));
			
			_meshes = new RN::Dictionary();
			
			if(preloadASCII)
			{
				for(int i = 0; i < 128; i++)
				{
					GetMeshForCharacter(i);
				}
			}
			Unlock();
		}

		Font::~Font()
		{
			Lock();
			delete _fontInfo;
			_fontData->Release();
			_meshes->Release();
			Unlock();
		}

		RN::Mesh *Font::GetMeshForCharacter(int codepoint)
		{
			Lock();
			RN::Mesh *existingMesh = _meshes->GetObjectForKey<RN::Mesh>(RN::Number::WithInt32(codepoint));
			if(existingMesh)
			{
				Unlock();
				return existingMesh;
			}
			
			stbtt_vertex *shapeVertices;
			int shapeVertexCount = stbtt_GetCodepointShape(_fontInfo, codepoint, &shapeVertices);
			//int shapeVertexCount = stbtt_GetGlyphShape(font, glyphIndex, &shapeVertices);
			
			KG::PathCollection paths;
			KG::Path mypath;
			KG::Vector2 from;
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
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return ascent;
		}

		float Font::GetDescent()
		{
			Lock();
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return descent;
		}

		float Font::GetLineOffset()
		{
			Lock();
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return linegap;
		}

		float Font::GetHeight()
		{
			Lock();
			int ascent, descent, linegap;
			stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &linegap);
			Unlock();
			
			return ascent - descent;
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
