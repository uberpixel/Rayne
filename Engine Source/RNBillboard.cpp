//
//  RNBillboard.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBillboard.h"
#include "RNResourceCoordinator.h"

#define kRNBillboardMeshResourceName RNCSTR("kRNBillboardMeshResourceName")

namespace RN
{
	RNDeclareMeta(Billboard)
	
	Billboard::Billboard()
	{
		_mesh = 0;
		_material = 0;
		
		Initialize();
	}
	
	Billboard::~Billboard()
	{
		_mesh->Release();
		_material->Release();
	}
	
	
	void Billboard::Initialize()
	{
		_material = new Material();
		_material->SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader, nullptr));
		_material->Define("RN_BILLBOARD");
		
		static std::once_flag onceFlag;
		
		std::call_once(onceFlag, []() {
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			
			Mesh *mesh = new Mesh(descriptors, 10, 0);
			mesh->SetMode(GL_TRIANGLE_STRIP);
			
			Mesh::Chunk chunk = mesh->GetChunk();
			
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(kMeshFeatureVertices);
			Mesh::ElementIterator<Vector2> uvCoords = chunk.GetIterator<Vector2>(kMeshFeatureUVSet0);
			
			*vertices ++ = Vector2(0.5f, 0.5f);
			*vertices ++ = Vector2(-0.5f, 0.5f);
			*vertices ++ = Vector2(0.5f, -0.5f);
			*vertices ++ = Vector2(-0.5f, -0.5f);
			
			*vertices ++ = Vector2(-0.5f, -0.5f);
			*vertices ++ = Vector2(-0.5f, 0.5f);
			
			*vertices ++ = Vector2(-0.5f, 0.5f);
			*vertices ++ = Vector2(0.5f, 0.5f);
			*vertices ++ = Vector2(-0.5f, -0.5f);
			*vertices ++ = Vector2(0.5f, -0.5f);
			
			
			*uvCoords ++ = Vector2(1.0f, 0.0f);
			*uvCoords ++ = Vector2(0.0f, 0.0f);
			*uvCoords ++ = Vector2(1.0f, 1.0f);
			*uvCoords ++ = Vector2(0.0f, 1.0f);
			
			*uvCoords ++ = Vector2(0.0f, 1.0f);
			*uvCoords ++ = Vector2(1.0f, 0.0f);
			
			*uvCoords ++ = Vector2(1.0f, 0.0f);
			*uvCoords ++ = Vector2(0.0f, 0.0f);
			*uvCoords ++ = Vector2(1.0f, 1.0f);
			*uvCoords ++ = Vector2(0.0f, 1.0f);
			
			chunk.CommitChanges();
			
			ResourceCoordinator::GetSharedInstance()->AddResource(mesh, kRNBillboardMeshResourceName);
		});
		
		_mesh = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Mesh>(kRNBillboardMeshResourceName, nullptr)->Retain();
	}
	
	void Billboard::SetTexture(Texture *texture)
	{
		_material->RemoveTextures();
		_material->AddTexture(texture);
		
		_size = Vector2(texture->GetWidth(), texture->GetHeight());
		_size *= 0.1f;
		
		SetBoundingBox(_mesh->GetBoundingBox() * Vector3(_size.x, _size.y, 1.0f));
	}
	
	void Billboard::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		_transform = GetWorldTransform();
		_transform.Scale(Vector3(_size.x, _size.y, 1.0f));
		
		RenderingObject object;
		FillRenderingObject(object);
		
		object.mesh = _mesh;
		object.material = _material;
		object.transform = &_transform;

		renderer->RenderObject(object);
	}
	
	Hit Billboard::CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		if(_mesh == 0)
			return hit;
		
		Matrix matModelInv = _transform.GetInverse();
		
		Vector3 temppos = matModelInv.Transform(position);
		Vector4 tempdir = matModelInv.Transform(Vector4(direction, 0.0f));
		
		hit = _mesh->IntersectsRay(temppos, Vector3(tempdir), mode);
		hit.node = this;
		
		return hit;
	}
}
