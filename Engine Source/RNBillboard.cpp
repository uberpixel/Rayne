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
	RNDefineMeta(Billboard, SceneNode)
	
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
	
	Billboard::Billboard(const Billboard *other) :
		SceneNode(other)
	{
		Billboard *temp = const_cast<Billboard *>(other);
		LockGuard<Object *> lock(temp);
		
		Initialize();
		
		_material->Release();
		_material = other->_material->Copy();
		
		_size = other->_size;
		
		SetBoundingBox(_mesh->GetBoundingBox() * Vector3(_size.x, _size.y, 1.0f));
	}
	
	Billboard::~Billboard()
	{
		_mesh->Release();
		_material->Release();
	}
	
	
	Billboard::Billboard(Deserializer *deserializer) :
		SceneNode(deserializer)
	{
		Initialize();
		SafeRelease(_material);
		
		_material = static_cast<Material *>(deserializer->DecodeObject())->Retain();
		_size = deserializer->DecodeVector2();
		
		SetBoundingBox(_mesh->GetBoundingBox() * Vector3(_size.x, _size.y, 1.0f));
	}
	
	void Billboard::Serialize(Serializer *serializer)
	{
		SceneNode::Serialize(serializer);
		
		serializer->EncodeObject(_material);
		serializer->EncodeVector2(_size);
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
			MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor uvDescriptor(MeshFeature::UVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			
			mesh = new Mesh(descriptors, 10, 0);
			mesh->SetDrawMode(Mesh::DrawMode::TriangleStrip);
			
			Mesh::Chunk chunk = mesh->GetChunk();
			
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(MeshFeature::Vertices);
			Mesh::ElementIterator<Vector2> uvCoords = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
			
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
			mesh->CalculateBoundingVolumes();
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
		
		Vector3 temppos = matModelInv * position;
		Vector4 tempdir = matModelInv * Vector4(direction, 0.0f);
		
		hit = _mesh->IntersectsRay(temppos, Vector3(tempdir), mode);
		hit.node = this;
		
		return hit;
	}
}
