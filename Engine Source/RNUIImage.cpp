//
//  RNUIImage.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImage.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Image)
		
		Image::Image(class Texture *texture) :
			_atlas(UI::Atlas(Vector2(0.0f), Vector2(1.0f)))
		{
			RN_ASSERT(texture, "Texture mustn't be NULL");
			_texture = texture->Retain();
		}
		
		Image::Image(const std::string& file) :
			_atlas(UI::Atlas(0.0f, 0.0f, 1.0f, 1.0f))
		{
			Texture::Parameter parameter;
			parameter.maxMipMaps = 0;
			
			_texture = Texture::WithFile(file, parameter);
			_texture->Retain();
		}
		
		Image::~Image()
		{
			_texture->Release();
		}
		
		
		Mesh *Image::GetFittingMesh(const Vector2& size, const Vector2& offset)
		{
			uint16 xverts = 2;
			uint16 yverts = 2;
			if(_insets.left > 0.0f)
				xverts += 1;
			if(_insets.right > 0.0f)
				xverts += 1;
			if(_insets.top > 0.0f)
				yverts += 1;
			if(_insets.bottom > 0.0f)
				yverts += 1;
			
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
			indicesDescriptor.elementMember = 1;
			indicesDescriptor.elementSize = sizeof(uint16);
	
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor, indicesDescriptor };
			Mesh *mesh = new Mesh(descriptors, xverts*yverts, (xverts-1)*(yverts-1)*3*2);
	
			Mesh::Chunk chunk  = mesh->GetChunk();
			Mesh::Chunk ichunk = mesh->GetIndicesChunk();
			
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(kMeshFeatureVertices);
			Mesh::ElementIterator<Vector2> uvCoords = chunk.GetIterator<Vector2>(kMeshFeatureUVSet0);
			Mesh::ElementIterator<uint16> indices   = ichunk.GetIterator<uint16>(kMeshFeatureIndices);
			
			
			for(uint16 x = 0; x < xverts; x++)
			{
				for(uint16 y = 0; y < yverts; y++)
				{
					float xpos;
					float ypos;
					float xuv;
					float yuv;
					if(x == 0)
					{
						xpos = 0.0f;
						xuv = 0.0f;
					}
					if(y == 0)
					{
						ypos = size.y;
						yuv = 0.0f;
					}
					
					if(x == xverts-1)
					{
						xpos = size.x;
						xuv = 1.0f;
					}
					if(y == yverts-1)
					{
						ypos = 0.0f;
						yuv = 1.0f;
					}
					
					if(_insets.left > 0.0f && x == 1)
					{
						xpos = _insets.left;
						xuv = _insets.left/GetWidth();
					}
					if(_insets.right > 0.0f && x == xverts-2)
					{
						xpos = size.x-_insets.right;
						xuv = 1.0f-_insets.right/GetWidth();
					}
					if(_insets.top > 0.0f && y == 1)
					{
						ypos = size.y-_insets.top;
						yuv = _insets.top/GetHeight();
					}
					if(_insets.bottom > 0.0f && y == yverts-2)
					{
						ypos = _insets.bottom;
						yuv = 1.0f-_insets.bottom/GetHeight();
					}
					
					*vertices ++ = Vector2(xpos, ypos) + offset;
					*uvCoords ++ = Vector2(xuv*(_atlas.u2-_atlas.u1)+_atlas.u1, yuv*(_atlas.v2-_atlas.v1)+_atlas.v1);
				}
			}
			
			for(uint16 x = 0; x < xverts-1; x++)
			{
				for(uint16 y = 0; y < yverts-1; y++)
				{
					*indices ++ = (x+1)*yverts+y;
					*indices ++ = x*yverts+y;
					*indices ++ = x*yverts+y+1;
					
					*indices ++ = (x+1)*yverts+y;
					*indices ++ = x*yverts+y+1;
					*indices ++ = (x+1)*yverts+y+1;
				}
			}
			
			chunk.CommitChanges();
			ichunk.CommitChanges();
			
			return mesh->Autorelease();
		}
		
		
		Image *Image::WithFile(const std::string& file)
		{
			Image *image = new Image(file);
			return image->Autorelease();
		}
		
		Image *Image::WithTexture(class Texture *texture)
		{
			Image *image = new Image(texture);
			return image->Autorelease();
		}
		
		void Image::SetAtlas(const Atlas& atlas, bool normalized)
		{
			_atlas = atlas;
			
			if(!normalized)
			{
				size_t width  = _texture->GetWidth();
				size_t height = _texture->GetHeight();
				
				_atlas.u1 /= width;
				_atlas.u2 /= width;
				
				_atlas.v1 /= height;
				_atlas.v2 /= height;
			}
		}
		
		void Image::SetEdgeInsets(const EdgeInsets& insets)
		{
			_insets = insets;
		}
		
		size_t Image::GetWidth() const
		{
			size_t width = _texture->GetWidth();
			float nwidth  = _atlas.u2 - _atlas.u1;
			
			width = nwidth * width;
			return width;
		}
		
		size_t Image::GetHeight() const
		{
			size_t height = _texture->GetHeight();
			float nheight = _atlas.v2 - _atlas.v1;
				
			height = nheight * height;
			return height;
		}
	}
}
