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
	RNDeclareMeta(Water)
	
	Water::Water(Camera *cam, Texture *refract)
	{
		_mesh = 0;
		_material = 0;
		_reflection = 0;
		_refraction = refract;
		_camera = cam;
		group = 2;
		
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
		
		if(_camera != 0)
		{
			
			
			_reflection = new Camera(Vector2(512, 512), TextureParameter::Format::RGBA8888, Camera::FlagFullscreen|Camera::FlagUpdateAspect|Camera::FlagUpdateStorageFrame|Camera::FlagHidden, RenderStorage::BufferFormatComplete, 1.0f);
			_reflection->SetPriority(100);
//			_reflection->SetSkyCube(_camera->SkyCube());
			
			
			RN::Shader *shad = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyTexture1Shader);
			
			RN::Material *mat = new RN::Material(shad);
			mat->lighting = false;
			mat->override |= Material::OverrideTextures;
			_reflection->SetMaterial(mat);
			_reflection->useclipplane = true;
			_reflection->clipplane = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
			
			_material->AddTexture(_reflection->Storage()->RenderTarget());
			_material->AddTexture(RN::Texture::WithFile("textures/waterbump.png", true));
			
			if(_refraction != 0)
				_material->AddTexture(_refraction);
		}
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
	
	void Water::Update(float delta)
	{
		SceneNode::Update(delta);
		
		if(_reflection != 0)
		{
			_reflection->SetPosition(Vector3(1.0f, -1.0f, 1.0f)*_camera->WorldPosition());
			Vector3 rot = _camera->WorldEulerAngle();
			_reflection->SetRotation(Vector3(rot.x, rot.y, -rot.z));
		}
	}
	
	bool Water::CanUpdate(FrameID frameid)
	{
		if(frameid < 5)
			return false;
		
		return true;
	}
	
	void Water::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
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
