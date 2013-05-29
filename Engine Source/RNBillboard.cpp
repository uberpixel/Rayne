//
//  RNBillboard.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBillboard.h"
#include "RNResourcePool.h"

#define kRNBillboardMeshResourceName "kRNBillboardMeshResourceName"

namespace RN
{
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
		_material = new RN::Material();
		_material->SetShader(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyTexture1Shader));
		_material->Define("RN_BILLBOARD");
		
		static std::once_flag onceFlag;
		
		std::call_once(onceFlag, []() {
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			vertexDescriptor.elementCount  = 10;
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			uvDescriptor.elementCount  = 10;
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			
			Mesh *mesh = new Mesh(descriptors);
			mesh->SetMode(GL_TRIANGLE_STRIP);
			
			Vector2 *vertices = mesh->Element<Vector2>(kMeshFeatureVertices);
			Vector2 *uvCoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
			
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
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->ReleaseElement(kMeshFeatureUVSet0);
			mesh->UpdateMesh();
			
			ResourcePool::SharedInstance()->AddResource(mesh, kRNBillboardMeshResourceName);
		});
		
		_mesh = ResourcePool::SharedInstance()->ResourceWithName<Mesh>(kRNBillboardMeshResourceName)->Retain();
	}
	
	void Billboard::SetTexture(Texture *texture)
	{
		_material->RemoveTextures();
		_material->AddTexture(texture);
		
		_size = Vector2(texture->Width(), texture->Height());
		_size *= 0.1f;
	}
	
	
	bool Billboard::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void Billboard::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		_transform = WorldTransform();
		_transform.Scale(Vector3(_size.x, _size.y, 1.0f));
		
		RenderingObject object;
		
		object.mesh = _mesh;
		object.material = _material;
		object.rotation = (Quaternion*)&WorldRotation();
		object.transform = &_transform;
		
		renderer->RenderObject(object);
	}
}
