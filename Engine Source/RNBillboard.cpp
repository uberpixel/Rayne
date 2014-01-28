//
//  RNBillboard.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBillboard.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	RNDeclareMeta(Billboard)
	
	Billboard::Billboard()
	{
		Initialize();
	}
	
	Billboard::Billboard(Texture *texture)
	{
		Initialize();
		SetTexture(texture);
	}
	
	Billboard::Billboard(Texture *texture, const Vector3 &position)
	{
		Initialize();
		SetTexture(texture);
		SetPosition(position);
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
		_material->SetBlending(true);
		_material->SetBlendMode(Material::BlendMode::SourceAlpha, Material::BlendMode::OneMinusSourceAlpha);
		
		static std::once_flag onceFlag;
		static Mesh *mesh;
		
		std::call_once(onceFlag, []() {
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			
			mesh = new Mesh(descriptors, 10, 0);
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
		});
		
		_mesh = mesh->Retain();
	}
	
	void Billboard::SetTexture(Texture *texture, float scaleFactor)
	{
		_material->RemoveTextures();
		
		if(texture)
		{
			_material->AddTexture(texture);
			
			_size = Vector2(texture->GetWidth(), texture->GetHeight());
			_size *= scaleFactor;
		}
		else
		{
			_size = Vector2();
		}
		
		SetBoundingBox(_mesh->GetBoundingBox() * Vector3(_size.x, _size.y, 1.0f));
	}
	
	Texture *Billboard::GetTexture() const
	{
		const Array *array = _material->GetTextures();
		return (array->GetCount() > 0) ? array->GetObjectAtIndex<Texture>(0) : nullptr;
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
		
		if(!_mesh)
			return hit;
		
		Matrix matModelInv = _transform.GetInverse();
		
		Vector3 temppos = matModelInv.Transform(position);
		Vector4 tempdir = matModelInv.Transform(Vector4(direction, 0.0f));
		
		hit = _mesh->IntersectsRay(temppos, Vector3(tempdir), mode);
		hit.node = this;
		
		return hit;
	}
}
