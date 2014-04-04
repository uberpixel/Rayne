//
//  RNTerrain.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTerrain.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	RNDefineMeta(Terrain, SceneNode)
	
	Terrain::Terrain()
	{
		Initialize();
	}
	
	Terrain::Terrain(const Terrain *other) :
		SceneNode(other)
	{
		Terrain *temp = const_cast<Terrain *>(other);
		LockGuard<Object *> lock(temp);
		
		//TODO: implement copy constructor
	}
	
	Terrain::~Terrain()
	{
		_mesh->Release();
		_material->Release();
	}
	
	
	Terrain::Terrain(Deserializer *deserializer) :
		SceneNode(deserializer)
	{
		Initialize();
		SafeRelease(_material);
		
		//TODO: handle deserialization
	}
	
	void Terrain::Serialize(Serializer *serializer)
	{
		SceneNode::Serialize(serializer);
		
		//TODO: handle serialization
	}
	
	
	void Terrain::Initialize()
	{
		_material = new Material();
		_material->SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDefaultShader, nullptr));
		
		static std::once_flag onceFlag;
		static Mesh *mesh;
		
		std::call_once(onceFlag, []() {
			MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
			vertexDescriptor.elementMember = 3;
			vertexDescriptor.elementSize   = sizeof(Vector3);
			
			MeshDescriptor uvDescriptor(MeshFeature::UVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			
			mesh = new Mesh(descriptors, 4, 0);
			mesh->SetDrawMode(Mesh::DrawMode::TriangleStrip);
			
			Mesh::Chunk chunk = mesh->GetChunk();
			
			Mesh::ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
			Mesh::ElementIterator<Vector2> uvCoords = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
			
			*vertices ++ = Vector3(-0.5f, 0.0f, 0.5f);
			*vertices ++ = Vector3(0.5f, 0.0f, 0.5f);
			*vertices ++ = Vector3(-0.5f, 0.0f, -0.5f);
			*vertices ++ = Vector3(0.5f, 0.0f, -0.5f);
			
			*uvCoords ++ = Vector2(0.0f, 0.0f);
			*uvCoords ++ = Vector2(1.0f, 0.0f);
			*uvCoords ++ = Vector2(0.0f, 1.0f);
			*uvCoords ++ = Vector2(1.0f, 1.0f);
			
			chunk.CommitChanges();
			mesh->CalculateBoundingVolumes();
		});
		
		_mesh = mesh->Retain();
	}
		
	void Terrain::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		_transform = GetWorldTransform();
		
		RenderingObject object;
		FillRenderingObject(object);
		
		object.mesh = _mesh;
		object.material = _material;
		object.transform = &_transform;

		renderer->RenderObject(object);
	}
	
	Hit Terrain::CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
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
