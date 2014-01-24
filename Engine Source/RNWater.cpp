//
//  RNWater.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWater.h"
#include "RNResourceCoordinator.h"

#define kRNWaterMeshResourceName RNCSTR("kRNWaterMeshResourceName")

namespace RN
{
	RNDeclareMeta(Water)
	
	Water::Water(Camera *cam, Texture *refract)
	{
		_mesh = 0;
		_material = 0;
		_reflection = 0;
		_refraction = refract;
		_camera = cam;
		renderGroup = 2;
		
		Initialize();
	}
	
	Water::~Water()
	{
		_mesh->Release();
		_material->Release();
	}
	
	
	void Water::Initialize()
	{
		SetPriority(SceneNode::Priority::UpdateLate);
		
		_material = new RN::Material();
		_material->SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyWaterShader, nullptr));
		
		static std::once_flag onceFlag;
		
		std::call_once(onceFlag, []() {
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 3;
			vertexDescriptor.elementSize   = sizeof(Vector3);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor };
			
			Mesh *mesh = new Mesh(descriptors, 10, 0);
			mesh->SetMode(GL_TRIANGLE_STRIP);
			
			Mesh::Chunk chunk = mesh->GetChunk();
			Mesh::ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(kMeshFeatureVertices);
			
			
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
			
			chunk.CommitChanges();
			
			ResourceCoordinator::GetSharedInstance()->AddResource(mesh, kRNWaterMeshResourceName);
		});
		
		_mesh = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Mesh>(kRNWaterMeshResourceName, nullptr)->Retain();
		
		if(_camera != 0)
		{
			_reflection = new Camera(Vector2(512, 512), Texture::Format::RGBA8888, Camera::Flags::UpdateStorageFrame | Camera::Flags::NoFlush, RenderStorage::BufferFormatComplete, 1.0f);
			_reflection->SetPriority(9);
			
			Shader *shad = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader, nullptr);
			
			Material *mat = new Material(shad);
			mat->lighting = false;
			
			_reflection->SetMaterial(mat);
			_reflection->SetFlags(_camera->GetFlags() | Camera::Flags::UseClipPlanes);
			_reflection->SetClipPlane(Vector4(0.0f, 1.0f, 0.0f, 0.0f));
			
			_material->AddTexture(_reflection->GetStorage()->GetRenderTarget());
			_material->AddTexture(RN::Texture::WithFile("textures/waterbump.png", true));
			
			if(_refraction != 0)
				_material->AddTexture(_refraction);
		}
	}
	
	void Water::SetTexture(Texture *texture)
	{
		_material->RemoveTextures();
		_material->AddTexture(texture);
		
		_size = Vector2(texture->GetWidth(), texture->GetHeight());
		_size *= 0.1f;
	}
	
	
	bool Water::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void Water::Update(float delta)
	{
		SceneNode::Update(delta);
		
		if(_reflection != 0)
		{
			Vector3 rot = _camera->GetWorldEulerAngle();
			
			_reflection->SetPosition(Vector3(1.0f, -1.0f, 1.0f)*_camera->GetWorldPosition());
			_reflection->SetRotation(Vector3(rot.x, rot.y, -rot.z));
			
			_reflection->SetAspectRatio(_camera->GetAspectRatio());
			_reflection->UpdateProjection();
		}
	}
	
	void Water::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		_transform = GetWorldTransform();
		_transform.Scale(Vector3(200.0f, 200.0f, 200.0f));
		
		RenderingObject object;
		FillRenderingObject(object);
		
		object.mesh = _mesh;
		object.material = _material;
		object.transform = &_transform;
		
		renderer->RenderObject(object);
	}
}
