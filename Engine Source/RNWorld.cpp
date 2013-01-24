//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"

namespace RN
{
	World::World(Kernel *kernel)
	{
		_kernel = kernel;
		_kernel->SetWorld(this);
		
		_cameras = new ObjectArray();
		_renderer = _kernel->Renderer();
		_physics = new PhysicsPipeline();
		
		Rect frame = _kernel->Window()->Frame();
		Camera *camera = new Camera(Vector2(frame.width, frame.height));
		
		_cameras->AddObject(camera);
		
		_physicsTask = kPipelineSegmentNullTask;
		_renderingTask = kPipelineSegmentNullTask;
		
		/*Shader *pptest1 = new Shader();
		pptest1->SetFragmentShader("shader/TestPP.fsh");
		pptest1->SetVertexShader("shader/TestPP.vsh");
		pptest1->Link();
		
		Shader *pptest2 = new Shader();
		pptest2->SetFragmentShader("shader/TestPP2.fsh");
		pptest2->SetVertexShader("shader/TestPP2.vsh");
		pptest2->Link();
		
		Material *ppmat1 = new Material(pptest1);
		Material *ppmat2 = new Material(pptest2);
		
		Camera *stage = new Camera(Vector2(frame.width, frame.height), Camera::FlagInherit | Camera::FlagDrawTarget);
		stage->SetMaterial(ppmat2);
		
		camera->SetMaterial(ppmat1);
		camera->AddStage(stage);*/
		
		camera->Release();
		
		
		CreateTestMesh();
	}
	
	World::~World()
	{
		_cameras->Release();
		delete _physics;
	}
	
	
	
	void World::Update(float delta)
	{
		Entity *entity = _entities[0];
		entity->Translate(Vector3(0.0f, 0.0f, -10.0f * delta));
	}
	
	void World::BeginUpdate(float delta)
	{
		_physicsTask = _physics->BeginTask(delta);
		
		Update(delta);
		
		for(auto i=_entities.begin(); i!=_entities.end(); i++)
		{
			Entity *entity = *i;
			entity->Update(delta);
		}
	}
	
	void World::FinishUpdate(float delta)
	{
		_physics->WaitForTaskCompletion(_physicsTask);
		
		for(auto i=_entities.begin(); i!=_entities.end(); i++)
		{
			Entity *entity = *i;
			entity->PostUpdate();
		}
		
		_renderer->WaitForTaskCompletion(_renderingTask);
		_renderingTask = _renderer->BeginTask(delta);
		
		for(machine_uint i=0; i<_cameras->Count(); i++)
		{
			Camera *camera = (Camera *)_cameras->ObjectAtIndex(i);
			
			RenderingGroup group;
			group.camera = camera;
			
			for(auto i=_entities.begin(); i!=_entities.end(); i++)
			{
				Entity *entity = *i;
				group.intents.push_back(entity->Intent());
			}
			
			_renderer->PushGroup(group);
		}
		
		_renderer->FinishFrame();
	}
	
	
	
	void World::AddEntity(Entity *entity)
	{
		_entities.push_back(entity);
	}
	
	void World::RemoveEntity(Entity *entity)
	{
		for(auto i=_entities.begin(); i!=_entities.end(); i++)
		{
			if(*i == entity)
			{
				_entities.erase(i);
				return;
			}
		}
	}
	
	
	void World::CreateTestMesh()
	{
		// Mesh
		Mesh *mesh = new Mesh();
		
		MeshDescriptor vertexDescriptor;
		vertexDescriptor.feature = kMeshFeatureVertices;
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 24;
		
		MeshDescriptor colorDescriptor;
		colorDescriptor.feature = kMeshFeatureColor0;
		colorDescriptor.elementSize = sizeof(Color);
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementCount  = 24;
		
		MeshDescriptor texcoordDescriptor;
		texcoordDescriptor.feature = kMeshFeatureUVSet0;
		texcoordDescriptor.elementSize = sizeof(Vector2);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementCount  = 24;
		
		MeshDescriptor indicesDescriptor;
		indicesDescriptor.feature = kMeshFeatureIndices;
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 36;
		
		Array<MeshDescriptor> descriptors;
		descriptors.AddObject(vertexDescriptor);
		descriptors.AddObject(colorDescriptor);
		descriptors.AddObject(indicesDescriptor);
		descriptors.AddObject(texcoordDescriptor);
		
		
		MeshLODStage *stage = mesh->AddLODStage(descriptors);
		
		Vector3 *vertices  = stage->Data<Vector3>(kMeshFeatureVertices);
		Color *colors      = stage->Data<Color>(kMeshFeatureColor0);
		Vector2 *texcoords = stage->Data<Vector2>(kMeshFeatureUVSet0);
		uint16 *indices    = stage->Data<uint16>(kMeshFeatureIndices);
		
		*vertices ++ = Vector3(-0.5f, 0.5f, 0.5f);
		*vertices ++ = Vector3(0.5f, 0.5f, 0.5f);
		*vertices ++ = Vector3(0.5f, -0.5f, 0.5f);
		*vertices ++ = Vector3(-0.5f, -0.5f, 0.5f);
		
		*vertices ++ = Vector3(-0.5f, 0.5f, -0.5f);
		*vertices ++ = Vector3(0.5f, 0.5f, -0.5f);
		*vertices ++ = Vector3(0.5f, -0.5f, -0.5f);
		*vertices ++ = Vector3(-0.5f, -0.5f, -0.5f);
		
		*vertices ++ = Vector3(-0.5f, 0.5f, -0.5f);
		*vertices ++ = Vector3(-0.5f, 0.5f, 0.5f);
		*vertices ++ = Vector3(-0.5f, -0.5f, 0.5f);
		*vertices ++ = Vector3(-0.5f, -0.5f, -0.5f);
		
		*vertices ++ = Vector3(0.5f, 0.5f, -0.5f);
		*vertices ++ = Vector3(0.5f, 0.5f, 0.5f);
		*vertices ++ = Vector3(0.5f, -0.5f, 0.5f);
		*vertices ++ = Vector3(0.5f, -0.5f, -0.5f);
		
		*vertices ++ = Vector3(-0.5f, 0.5f, 0.5f);
		*vertices ++ = Vector3(0.5f, 0.5f, 0.5f);
		*vertices ++ = Vector3(0.5f, 0.5f, -0.5f);
		*vertices ++ = Vector3(-0.5f, 0.5f, -0.5f);
		
		*vertices ++ = Vector3(-0.5f, -0.5f, -0.5f);
		*vertices ++ = Vector3(0.5f, -0.5f, -0.5f);
		*vertices ++ = Vector3(0.5f, -0.5f, 0.5f);
		*vertices ++ = Vector3(-0.5f, -0.5f, 0.5f);
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*texcoords ++ = Vector2(0.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 0.0f);
		*texcoords ++ = Vector2(1.0f, 1.0f);
		*texcoords ++ = Vector2(0.0f, 1.0f);
		
		*colors ++ = Color(1.0f, 0.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 1.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 1.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 0.0f, 1.0f);
		
		*colors ++ = Color(1.0f, 0.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 1.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 1.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 0.0f, 1.0f);
		
		*colors ++ = Color(1.0f, 0.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 1.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 1.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 0.0f, 1.0f);
		
		*colors ++ = Color(1.0f, 0.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 1.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 1.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 0.0f, 1.0f);
		
		*colors ++ = Color(1.0f, 0.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 1.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 1.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 0.0f, 1.0f);
		
		*colors ++ = Color(1.0f, 0.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 1.0f, 0.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 1.0f, 1.0f);
		*colors ++ = Color(0.0f, 0.0f, 0.0f, 1.0f);
		
		*indices ++ = 0;
		*indices ++ = 3;
		*indices ++ = 1;
		*indices ++ = 2;
		*indices ++ = 1;
		*indices ++ = 3;
		
		*indices ++ = 4;
		*indices ++ = 7;
		*indices ++ = 5;
		*indices ++ = 6;
		*indices ++ = 5;
		*indices ++ = 7;
		
		*indices ++ = 8;
		*indices ++ = 11;
		*indices ++ = 9;
		*indices ++ = 10;
		*indices ++ = 9;
		*indices ++ = 11;
		
		*indices ++ = 12;
		*indices ++ = 15;
		*indices ++ = 13;
		*indices ++ = 14;
		*indices ++ = 13;
		*indices ++ = 15;
		
		*indices ++ = 16;
		*indices ++ = 19;
		*indices ++ = 17;
		*indices ++ = 18;
		*indices ++ = 17;
		*indices ++ = 19;
		
		*indices ++ = 20;
		*indices ++ = 23;
		*indices ++ = 21;
		*indices ++ = 22;
		*indices ++ = 21;
		*indices ++ = 23;
		
		mesh->UpdateMesh();
		
		// Shader
		Texture *texture0 = new Texture("textures/brick.png", Texture::FormatRGB565);
		Texture *texture1 = new Texture("textures/testpng.png", Texture::FormatRGB565);
		
		Shader *shader = new Shader();
#if !RN_PLATFORM_IOS
//		shader->SetGeometryShader("shader/Test.gsh");
#endif
		shader->SetFragmentShader("shader/Test.fsh");
		shader->SetVertexShader("shader/Test.vsh");
		
		shader->Link();
		
		// Material
		Material *material = new Material(shader);
		material->AddTexture(texture0);
		material->AddTexture(texture1);
		material->culling = false;
		
		// Entity
		RigidBodyEntity *entity = new RigidBodyEntity();
		entity->SetMesh(mesh);
		entity->SetMaterial(material);
		Quaternion rot;
		rot.MakeEulerAngle(Vector3(45.0f, 45.0f, 45.0f));
		entity->SetRotation(rot);
		entity->SetPosition(Vector3(0.0f, 8.0f, -8.0f));
		entity->SetSize(Vector3(0.5f, 0.5f, 0.5f));
		entity->SetMass(10.0f);
		_physics->AddRigidBody(entity);
		
		entity = new RigidBodyEntity();
		entity->SetMesh(mesh);
		entity->SetMaterial(material);
		rot.MakeEulerAngle(Vector3(45.0f, 45.0f, 45.0f));
		entity->SetRotation(rot);
		entity->SetPosition(Vector3(1.0f, 8.0f, -8.0f));
		entity->SetSize(Vector3(0.5f, 0.5f, 0.5f));
		entity->SetMass(10.0f);
		_physics->AddRigidBody(entity);
		
		entity = new RigidBodyEntity();
		entity->SetMesh(mesh);
		entity->SetMaterial(material);
		rot.MakeEulerAngle(Vector3(45.0f, 45.0f, 45.0f));
		entity->SetRotation(rot);
		entity->SetPosition(Vector3(-0.25f, 5.0f, -8.0f));
		entity->SetSize(Vector3(0.5f, 0.5f, 0.5f));
		entity->SetMass(10.0f);
		_physics->AddRigidBody(entity);
		
		
		shader = new Shader();
		shader->SetFragmentShader("shader/Ground.fsh");
		shader->SetVertexShader("shader/Ground.vsh");
		shader->Link();
		
		texture0 = new Texture("textures/tiles.png", Texture::FormatRGB565);
		material = new Material(shader);
		material->AddTexture(texture0);
		material->AddTexture(texture1);
		material->culling = false;
		
		entity = new RigidBodyEntity();
		entity->SetMesh(mesh);
		entity->SetMaterial(material);
		entity->SetPosition(Vector3(0.0f, -5.0f, -8.0f));
		entity->SetScale(Vector3(10.0f, 1.0f, 10.0f));
		entity->SetMass(0.0f);
		entity->SetSize(Vector3(5.0f, 0.5f, 5.0f));
		_physics->AddRigidBody(entity);
	}
}
