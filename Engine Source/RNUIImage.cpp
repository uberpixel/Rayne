//
//  RNUIImage.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
			RN_ASSERT0(texture);
			_texture = texture->Retain();
		}
		
		Image::Image(const std::string& file) :
			_atlas(UI::Atlas(0.0f, 0.0f, 1.0f, 1.0f))
		{
			TextureParameter parameter;
			parameter.mipMaps = 0;
			parameter.generateMipMaps = false;
			
			_texture = new RN::Texture(file, parameter, true);
		}
		
		Image::~Image()
		{
			_texture->Release();
		}
		
		
		Mesh *Image::FittingMesh(const Vector2& size)
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
			vertexDescriptor.elementCount  = xverts*yverts;
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			uvDescriptor.elementCount  = xverts*yverts;
			
			MeshDescriptor indicesDescriptor(kMeshFeatureIndices);
			indicesDescriptor.elementMember = 1;
			indicesDescriptor.elementSize = sizeof(uint16);
			indicesDescriptor.elementCount = (xverts-1)*(yverts-1)*3*2;
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor, indicesDescriptor };
			Mesh *mesh = new Mesh(descriptors);
			mesh->SetMode(GL_TRIANGLES);
			
			Vector2 *vertices = mesh->Element<Vector2>(kMeshFeatureVertices);
			Vector2 *uvCoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
			uint16 *indices = mesh->Element<uint16>(kMeshFeatureIndices);
			
			for(uint16 x = 0; x < xverts; x++)
			{
				for(uint16 y = 0; y < yverts; y++)
				{
					float xpos;
					float ypos;
					if(x == 0)
						xpos = 0.0f;
					if(y == 0)
						ypos = size.y;
					
					if(x == xverts-1)
						xpos = size.x;
					if(y == yverts-1)
						ypos = 0.0f;
					
					if(_insets.left > 0.0f && x == 1)
						xpos = _insets.left;
					if(_insets.right > 0.0f && x == xverts-2)
						xpos = size.x-_insets.right;
					if(_insets.top > 0.0f && y == 1)
						ypos = size.y-_insets.top;
					if(_insets.bottom > 0.0f && y == yverts-2)
						ypos = _insets.bottom;
					
					*vertices ++ = Vector2(xpos, ypos);
					
					*uvCoords ++ = Vector2(xpos/Width()/(_atlas.u2-_atlas.u1)+_atlas.u1, 1.0-ypos/Height()/(_atlas.v2-_atlas.v1)+_atlas.v1);
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
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->ReleaseElement(kMeshFeatureUVSet0);
			mesh->ReleaseElement(kMeshFeatureIndices);
			mesh->UpdateMesh();
				
			return mesh->Autorelease();
		}
		
		
		Image *Image::WithFile(const std::string& file)
		{
			Image *image = new Image(file);
			return image->Autorelease();
		}
		
		
		void Image::SetAtlas(const struct Atlas& atlas, bool normalized)
		{
//			_mutated = true;
			_atlas   = atlas;
			
			if(!normalized)
			{
				uint32 width = Width(false);
				uint32 height = Height(false);
				
				_atlas.u1 /= width;
				_atlas.u2 = (width - _atlas.u2) / width;
				
				_atlas.v1 /= height;
				_atlas.v2 = (height - _atlas.v2) / height;
			}
		}
		
		void Image::SetEdgeInsets(const EdgeInsets& insets)
		{
			_insets  = insets;
//			_mutated = true;
		}
		
		uint32 Image::Width(bool atlasApplied) const
		{
			uint32 width = _texture->Width();
			if(atlasApplied)
			{
				float nwidth  = _atlas.u2 - _atlas.u1;
				width = nwidth * width;
			}
			
			return width;
		}
		
		uint32 Image::Height(bool atlasApplied) const
		{
			uint32 height = _texture->Height();
			if(atlasApplied)
			{
				float nheight = _atlas.v2 - _atlas.v1;
				height = nheight * height;
			}
			
			return height;
		}
	}
}
