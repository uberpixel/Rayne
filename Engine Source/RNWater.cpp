//
//  RNWater.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWater.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	RNDefineMeta(Water)
	
	Water::Water(Camera *cam, Texture *refract) :
		_mesh(nullptr),
		_material(nullptr),
		_reflection(nullptr),
		_refraction(refract->Retain()),
		_camera(cam->Retain())
	{
		Initialize();
		SetRenderGroup(2);
	}
	
	Water::~Water()
	{
		_refraction->Release();
		_camera->Release();
		
		_mesh->Release();
		_material->Release();
	}
	
	
	void Water::Initialize()
	{
		SetPriority(SceneNode::Priority::UpdateLate);
		
		_material = new RN::Material();
		_material->SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyWaterShader, nullptr));
		
		static std::once_flag onceFlag;
		static Mesh *mesh;
		
		std::call_once(onceFlag, [&]() {
			MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
			vertexDescriptor.elementMember = 3;
			vertexDescriptor.elementSize   = sizeof(Vector3);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor };
			
			mesh = new Mesh(descriptors, 10, 0);
			mesh->SetDrawMode(Mesh::DrawMode::TriangleStrip);
			
			Mesh::Chunk chunk = mesh->GetChunk();
			Mesh::ElementIterator<Vector3> vertices = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
			
			
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

		});
		
		_mesh = mesh->Retain();
		
		if(_camera != 0)
		{
			_reflection = new Camera(Vector2(512, 512), Texture::Format::RGBA8888, Camera::Flags::UpdateStorageFrame | Camera::Flags::NoFlush | Camera::Flags::UseClipPlanes, RenderStorage::BufferFormatComplete, 1.0f);
			_reflection->SetPriority(9);
			_reflection->SetRenderGroups(_reflection->GetRenderGroups() | Camera::RenderGroups::Group3);
			
			Shader *shad = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader, nullptr);
			
			Material *mat = new Material(shad);
			mat->SetOverride(0xffffffff & ~Material::Override::Shader);
			
			_reflection->SetMaterial(mat);
			_reflection->SetClipPlane(RN::Plane());
			
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
			_reflection->SetRotation(Vector3(rot.x, -rot.y, rot.z));
			
			_reflection->SetAspectRatio(_camera->GetAspectRatio());
			
			_reflection->SetSky(_camera->GetSky());
			_reflection->SetFogColor(_camera->GetFogColor());
			_reflection->SetFogNear(_camera->GetFogNear());
			_reflection->SetFogFar(_camera->GetFogFar());
			
			if(_camera->GetFlags() & Camera::Flags::UseFog)
			_reflection->SceneNode::SetFlags(_reflection->GetFlags() | Camera::Flags::UseFog);
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
