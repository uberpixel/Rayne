//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
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
		
		Rect frame = _kernel->Window()->Frame();
		Camera *camera = new Camera(Vector2(frame.width, frame.height));
		
		_cameras->AddObject(camera);
		camera->Release();
		
		CreateTestMesh();
	}
	
	World::~World()
	{
		_cameras->Release();
	}
	
	
	void World::Update(float delta)
	{
		static float rot = 0;
		static float dist = -128.0f;
		
		rot += 1.0f;
		//dist -= 1.0f;
		
		transform.MakeTranslate(Vector3(0.0f, 0.0f, dist));
		transform.Rotate(Vector3(0.0f, rot, -rot));
		
		for(machine_uint i=0; i<_cameras->Count(); i++)
		{
			Camera *camera = (Camera *)_cameras->ObjectAtIndex(i);
			
			RenderingIntent intent;
			intent.transform = transform;
			intent.mesh      = mesh;
			intent.material  = material;
			
			RenderingGroup group;
			group.camera = camera;
			group.intents.push_back(intent);
			
			_renderer->PushGroup(group);
		}
	}
	
	
	
	void World::CreateTestMesh()
	{
		// Mesh
		mesh = new Mesh();
		
		MeshDescriptor vertexDescriptor;
		vertexDescriptor.feature = kMeshFeatureVertices;
		vertexDescriptor.elementSize = sizeof(Vector3);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementCount  = 4;
		
		MeshDescriptor colorDescriptor;
		colorDescriptor.feature = kMeshFeatureColor0;
		colorDescriptor.elementSize = sizeof(Color);
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementCount  = 4;
		
		MeshDescriptor indicesDescriptor;
		indicesDescriptor.feature = kMeshFeatureIndices;
		indicesDescriptor.elementSize = sizeof(uint16);
		indicesDescriptor.elementMember = 1;
		indicesDescriptor.elementCount  = 6;
		
		Array<MeshDescriptor> descriptors;
		descriptors.AddObject(vertexDescriptor);
		descriptors.AddObject(colorDescriptor);
		descriptors.AddObject(indicesDescriptor);
		
		
		MeshLODStage *stage = mesh->AddLODStage(descriptors);
		
		Vector3 *vertices = stage->Data<Vector3>(kMeshFeatureVertices);
		Color *colors     = stage->Data<Color>(kMeshFeatureColor0);
		uint16 *indices   = stage->Data<uint16>(kMeshFeatureIndices);
		
		*vertices ++ = Vector3(-32.0f, 32.0f, 0.0f);
		*vertices ++ = Vector3(32.0f, 32.0f, 0.0f);
		*vertices ++ = Vector3(32.0f, -32.0f,  0.0f);
		*vertices ++ = Vector3(-32.0f, -32.0f,  0.0f);
		
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
		
		stage->GenerateMesh();
		
		// Shader
		shader = new Shader();
		shader->SetFragmentShader("shader/Test.fsh");
		shader->SetVertexShader("shader/Test.vsh");
		
		shader->Link();
		
		// Material
		material = new Material(shader);
		material->culling = false;
	}
}
