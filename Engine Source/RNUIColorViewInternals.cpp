//
//  RNUIColorViewInternals.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIColorViewInternals.h"
#include "RNUIColor.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	namespace UI
	{
		AlphaBackground::AlphaBackground()
		{
			_material = GetBasicMaterial(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(RNCSTR("shader/rn_UIAlphaBackground"), nullptr));
			_material->Retain();
			
			MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor texcoordDescriptor(MeshFeature::UVSet0);
			texcoordDescriptor.elementMember = 2;
			texcoordDescriptor.elementSize   = sizeof(Vector2);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, texcoordDescriptor };
			
			_mesh = new Mesh(descriptors, 6, 0);
			_mesh->SetDrawMode(Mesh::DrawMode::Triangles);
			
			Mesh::Chunk chunk = _mesh->GetChunk();
			
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(MeshFeature::Vertices);
			Mesh::ElementIterator<RN::Vector2> texcoords = chunk.GetIterator<RN::Vector2>(MeshFeature::UVSet0);
			
			Vector2 size = GetBounds().GetSize();
			
			*vertices ++ = Vector2(size.x, size.y);
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(size.x, 0.0f);
			
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(0.0f, 0.0f);
			*vertices ++ = Vector2(size.x, 0.0f);
			
			*texcoords ++ = Vector2(1.0f, 0.0f);
			*texcoords ++ = Vector2(0.0f, 0.0f);
			*texcoords ++ = Vector2(1.0f, 1.0f);
			*texcoords ++ = Vector2(0.0f, 0.0f);
			*texcoords ++ = Vector2(0.0f, 1.0f);
			*texcoords ++ = Vector2(1.0f, 1.0f);
			
			chunk.CommitChanges();
		}
		
		void AlphaBackground::SetFrame(const Rect &frame)
		{
			View::SetFrame(frame);
			
			Mesh::Chunk chunk = _mesh->GetChunk();
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(MeshFeature::Vertices);
			
			Vector2 size = GetBounds().GetSize();
			
			*vertices ++ = Vector2(size.x, size.y);
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(size.x, 0.0f);
			
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(0.0f, 0.0f);
			*vertices ++ = Vector2(size.x, 0.0f);
			
			chunk.CommitChanges();
		}
		
		void AlphaBackground::Draw(Renderer *renderer)
		{
			View::Draw(renderer);
			
			RenderingObject object;
			PopulateRenderingObject(object);
			
			object.mesh = _mesh;
			object.material = _material;
			
			renderer->RenderObject(object);
		}
	}
}
