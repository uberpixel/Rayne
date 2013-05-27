//
//  RNWater.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWater.h"
#include "RNResourcePool.h"

#define kRNWaterMeshResourceName "kRNWaterMeshResourceName"

namespace RN
{
	Water::Water()
	{
		_mesh = 0;
		_material = 0;
		
		Initialize();
	}
	
	Water::~Water()
	{
		_mesh->Release();
		_material->Release();
	}
	
	
	void Water::Initialize()
	{
		_material = new RN::Material();
		_material->SetShader(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyWaterShader));
		
		static std::once_flag onceFlag;
		
		std::call_once(onceFlag, []() {
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 3;
			vertexDescriptor.elementSize   = sizeof(Vector3);
			vertexDescriptor.elementCount  = 10;
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor };
			
			Mesh *mesh = new Mesh(descriptors);
			mesh->SetMode(GL_TRIANGLE_STRIP);
			
			Vector3 *vertices = mesh->Element<Vector3>(kMeshFeatureVertices);
			
			*vertices ++ = Vector3(0.5f, 0.0f, 0.5f);
			*vertices ++ = Vector3(-0.5f, 0.0f, 0.5f);
			*vertices ++ = Vector3(0.5f, 0.0f, -0.5f);
			*vertices ++ = Vector3(-0.5f, 0.0f, -0.5f);
			
			*vertices ++ = Vector3(-0.5f, 0.0f, -0.5f);
			*vertices ++ = Vector3(-0.5f, 0.0f, 0.5f);
			
			*vertices ++ = Vector3(-0.5f, 0.0f, 0.5f);
			*vertices ++ = Vector3(0.5f, 0.0f, 0.5f);
			*vertices ++ = Vector3(-0.5f, 0.0f, -0.5f);
			*vertices ++ = Vector3(0.5f, 0.0f, -0.5f);
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->UpdateMesh();
			
			ResourcePool::SharedInstance()->AddResource(mesh, kRNWaterMeshResourceName);
		});
		
		_mesh = ResourcePool::SharedInstance()->ResourceWithName<Mesh>(kRNWaterMeshResourceName)->Retain();
	}
	
	void Water::SetTexture(Texture *texture)
	{
		_material->RemoveTextures();
		_material->AddTexture(texture);
		
		_size = Vector2(texture->Width(), texture->Height());
		_size *= 0.1f;
	}
	
	
	bool Water::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void Water::Render(Renderer *renderer, Camera *camera)
	{
		_transform = WorldTransform();
		_transform.Scale(Vector3(200.0f, 200.0f, 200.0f));
		
		RenderingObject object;
		
		object.mesh = _mesh;
		object.material = _material;
		object.rotation = (Quaternion*)&WorldRotation();
		object.transform = &_transform;
		
		renderer->RenderObject(object);
	}
}
