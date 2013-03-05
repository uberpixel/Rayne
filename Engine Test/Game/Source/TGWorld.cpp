//
//  TGWorld.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGWorld.h"

namespace TG
{
	World::World()
	{
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatColor | RN::RenderStorage::BufferFormatDepth | RN::RenderStorage::BufferFormatStencil);
		RN::Texture *depthtex = new RN::Texture(RN::Texture::FormatDepthStencil);
		
		storage->SetDepthTarget(depthtex);
		storage->AddRenderTarget(RN::Texture::FormatRGBA8888);
		
		_camera = new RN::Camera(RN::Vector2(), storage, RN::Camera::FlagDefaults);
		_camera->SetClearColor(RN::Color(0.0, 0.0, 0.0, 1.0));
		_camera->ActivateTiledLightLists((RN::Texture *)1);
		_camera->SetSkyCube(RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png"));
				
		CreateWorld();
		
		RN::Input::SharedInstance()->Activate();
	}
	
	World::~World()
	{
		RN::Input::SharedInstance()->Deactivate();
		
		_camera->Release();
	}
	
	void World::Update(float delta)
	{
		RN::Input *input = RN::Input::SharedInstance();
		RN::Vector3 translation;
		RN::Vector3 rotation;
		
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS || RN_PLATFORM_LINUX
		const RN::Vector3& mouseDelta = input->MouseDelta() * -0.2f;
		
		rotation.x = mouseDelta.x;
		rotation.z = mouseDelta.y;
		
		translation.x = (input->KeyPressed('a') - input->KeyPressed('d')) * 18.0f;
		translation.z = (input->KeyPressed('w') - input->KeyPressed('s')) * 18.0f;
#endif
		
#if RN_PLATFORM_IOS
		const std::vector<RN::Touch>& touches = input->Touches();
		
		for(auto i=touches.begin(); i!=touches.end(); i++)
		{
			if(i->phase == RN::Touch::TouchPhaseBegan)
			{
				if(i->location.x > _camera->Frame().width*0.5f)
				{
					_touchRight = i->uniqueID;
				}
				else
				{
					_touchLeft = i->uniqueID;
				}
			}
			
			if(i->phase == RN::Touch::TouchPhaseMoved)
			{
				if(i->uniqueID == _touchRight)
				{
					rotation.x = i->initialLocation.x - i->location.x;
					rotation.z = i->initialLocation.y - i->location.y;
				}
						
				if(i->uniqueID == _touchLeft)
				{
					translation.x = i->initialLocation.x - i->location.x;
					translation.z = i->initialLocation.y - i->location.y;
				}
			}
		}
		
		rotation *= delta;
		translation *= 0.2f;
#endif
		
		_parentBlock->Rotate(RN::Vector3(0.0f, 64.0f * delta, 0.0f));
		
		_childBlock->Rotate(RN::Vector3(32.0f * delta, 0.0f, 32.0f * delta));
		_childBlock->SetPosition(RN::Vector3(0.0f, 2.0f + (sinf(RN::Kernel::SharedInstance()->Time())), 0.0f));
		
		_camera->Rotate(rotation);
		
		RN::Matrix rot;
		rot.MakeRotate(_camera->Rotation());
		_camera->Translate(rot.Transform(translation * -delta));
		
#if !(RN_PLATFORM_IOS)
		//_finalcam->SetPosition(_camera->Position());
		//_finalcam->SetRotation(_camera->Rotation());
#endif
	}
	
	void World::CreateWorld()
	{
		RN::Shader *shader = RN::Shader::WithFile("shader/rn_Texture1Discard");
		RN::Shader *lightshader = RN::Shader::WithFile("shader/rn_Texture1");
		
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		for(int i = 0; i < model->Meshes(); i++)
		{
			model->MaterialForMesh(model->MeshAtIndex(i))->SetShader(lightshader);
		}
		model->MaterialForMesh(model->MeshAtIndex(4))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(4))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(4))->alphatest = true;
		model->MaterialForMesh(model->MeshAtIndex(10))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(10))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(10))->alphatest = true;
		model->MaterialForMesh(model->MeshAtIndex(19))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(19))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(19))->alphatest = true;
		
		RN::Entity *sponza = new RN::Entity();
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.1, 0.1, 0.1));
		sponza->Rotate(RN::Vector3(0.0, 0.0, -90.0));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
		RN::Texture *blockTexture0 = RN::Texture::WithFile("textures/brick.png", RN::Texture::FormatRGB888);
		
		RN::Material *blockMaterial = new RN::Material(lightshader);
		blockMaterial->AddTexture(blockTexture0);
		
		RN::Mesh  *blockMesh = RN::Mesh::CubeMesh(RN::Vector3(0.5f, 0.5f, 0.5f));
		RN::Model *blockModel = RN::Model::WithMesh(blockMesh->Autorelease<RN::Mesh>(), blockMaterial->Autorelease<RN::Material>());
		
		_parentBlock = new RN::Entity();
		_parentBlock->SetModel(blockModel);
		
		_childBlock = new RN::Entity();
		_childBlock->SetModel(blockModel);
		
		_parentBlock->AttachChild(_childBlock);
		
#if 0
		RN::LightEntity *light;
		
		light = new RN::LightEntity();
		light->SetPosition(RN::Vector3(-30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Vector3((float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX));
		
		light = new RN::LightEntity();
		light->SetPosition(RN::Vector3(30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Vector3((float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX));
		 
		
		for(int i = 0; i < 1000; i++)
		{
			light = new RN::LightEntity();
			light->SetPosition(RN::Vector3((float)(rand())/RAND_MAX*280.0f-140.0f, (float)(rand())/RAND_MAX*100.0f, (float)(rand())/RAND_MAX*120.0f-50.0f));
			light->SetRange((float)(rand())/RAND_MAX*20.0f);
			light->SetColor(RN::Vector3((float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX));
		}
#endif
		
#if 0 //RN_TARGET_OPENGL
		RN::Shader *instancedShader = RN::Shader::WithFile("shader/rn_Texture1DiscardLight_instanced");
		RN::Model *foliage[4];
		
		foliage[0] = RN::Model::WithFile("models/nobiax/fern_01.sgm");
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->culling = false;
		
		foliage[1] = RN::Model::WithFile("models/nobiax/grass_05.sgm");
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->culling = false;
		
		foliage[2] = RN::Model::WithFile("models/nobiax/grass_19.sgm");
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->culling = false;
		
		foliage[3] = RN::Model::WithFile("models/nobiax/grass_04.sgm");
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->culling = false;
		
		uint32 index = 0;
		
		for(float x = -100.0f; x < 200.0f; x += 1.5f)
		{
			for(float y = -10.0f; y < 10.0f; y += 1.0f)
			{
				index = (index + 1) % 4;
				
				RN::Entity *fern = new RN::Entity();
				fern->SetModel(foliage[index]);
				fern->Rotate(RN::Vector3(0.0, 0.0, -90.0));
				fern->SetPosition(RN::Vector3(x, -5.3, y));
			}
		}
#endif
	}
}
