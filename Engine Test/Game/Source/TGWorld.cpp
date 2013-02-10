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
#if !(RN_PLATFORM_IOS)
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatDepth|RN::RenderStorage::BufferFormatStencil);
		RN::Texture *depthtex = new RN::Texture(RN::Texture::FormatR8);
		storage->SetDepthTexture(depthtex);
		
		RN::Shader *surfaceShader = RN::Shader::WithFile("shader/SurfaceNormals");
		RN::Material *surfaceMaterial = new RN::Material(surfaceShader);
		
		_camera = new RN::Camera(RN::Vector2(), storage, RN::Camera::FlagDefaults);
		_camera->SetClearColor(RN::Color(0.0, 0.0, 0.0, 1.0));
		_camera->SetMaterial(surfaceMaterial);
		
		RN::Shader *downsampleShader = RN::Shader::WithFile("shader/rn_LightTileSample");
		RN::Shader *downsampleFirstShader = RN::Shader::WithFile("shader/rn_LightTileSampleFirst");
		RN::Material *downsampleMaterial2x = new RN::Material(downsampleFirstShader);
		downsampleMaterial2x->AddTexture(depthtex);
		
		RN::Camera *downsample2x = new RN::Camera(RN::Vector2(_camera->Frame().width/2, _camera->Frame().height/2), RN::Texture::FormatRGB32F, RN::Camera::FlagDrawTarget | RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection);
		_camera->AddStage(downsample2x);
		downsample2x->SetMaterial(downsampleMaterial2x);
		
		RN::Material *downsampleMaterial4x = new RN::Material(downsampleShader);
		downsampleMaterial4x->AddTexture(downsample2x->Storage()->RenderTarget());
		
		RN::Camera *downsample4x = new RN::Camera(RN::Vector2(_camera->Frame().width/4, _camera->Frame().height/4), RN::Texture::FormatRGB32F, RN::Camera::FlagDrawTarget | RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection);
		_camera->AddStage(downsample4x);
		downsample4x->SetMaterial(downsampleMaterial4x);
		
		RN::Material *downsampleMaterial8x = new RN::Material(downsampleShader);
		downsampleMaterial8x->AddTexture(downsample4x->Storage()->RenderTarget());
		
		RN::Camera *downsample8x = new RN::Camera(RN::Vector2(_camera->Frame().width/8, _camera->Frame().height/8), RN::Texture::FormatRGB32F, RN::Camera::FlagDrawTarget | RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection);
		_camera->AddStage(downsample8x);
		downsample8x->SetMaterial(downsampleMaterial8x);
		
		RN::Material *downsampleMaterial16x = new RN::Material(downsampleShader);
		downsampleMaterial16x->AddTexture(downsample8x->Storage()->RenderTarget());
		
		RN::Camera *downsample16x = new RN::Camera(RN::Vector2(_camera->Frame().width/16, _camera->Frame().height/16), RN::Texture::FormatRGB32F, RN::Camera::FlagDrawTarget | RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection);
		_camera->AddStage(downsample16x);
		downsample16x->SetMaterial(downsampleMaterial16x);
		
		RN::Material *downsampleMaterial32x = new RN::Material(downsampleShader);
		downsampleMaterial32x->AddTexture(downsample16x->Storage()->RenderTarget());
		
		RN::Camera *downsample32x = new RN::Camera(RN::Vector2(_camera->Frame().width/32, _camera->Frame().height/32), RN::Texture::FormatRGB32F, RN::Camera::FlagDrawTarget | RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection);
		_camera->AddStage(downsample32x);
		downsample32x->SetMaterial(downsampleMaterial32x);
		
		RN::Camera *downsample64x;
		if(RN::Kernel::SharedInstance()->ScaleFactor() == 2.0f)
		{
			RN::Material *downsampleMaterial64x = new RN::Material(downsampleShader);
			downsampleMaterial64x->AddTexture(downsample32x->Storage()->RenderTarget());
			
			downsample64x = new RN::Camera(RN::Vector2(_camera->Frame().width/64, _camera->Frame().height/64), RN::Texture::FormatRGB32F, RN::Camera::FlagDrawTarget | RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection);
			_camera->AddStage(downsample64x);
			downsample64x->SetMaterial(downsampleMaterial64x);
		}
			
		_finalcam = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGBA8888, RN::Camera::FlagDefaults | RN::Camera::FlagInheritPosition | RN::Camera::FlagInheritProjection | RN::Camera::FlagNoClear);
		_finalcam->Storage()->SetDepthTexture(depthtex);
		_finalcam->SetName("Final Cam");
//		_camera->AddStage(_finalcam);
		
		if(RN::Kernel::SharedInstance()->ScaleFactor() == 2.0f)
		{
			_finalcam->ActivateTiledLightLists(downsample64x->Storage()->RenderTarget());
		}
		else
		{
			_finalcam->ActivateTiledLightLists(downsample32x->Storage()->RenderTarget());
		}
#else
		_camera = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGBA8888, RN::Camera::FlagDefaults);
		_camera->SetClearColor(RN::Color(0.0, 0.0, 0.0, 1.0));
#endif
		
		CreateWorld();
		
#if RN_PLATFORM_MAC_OS
//		CreateSSAOStage();
#endif
		
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
		
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
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
		
		_camera->Rotate(rotation);
		
		RN::Matrix rot;
		rot.MakeRotate(_camera->Rotation());
		_camera->Translate(rot.Transform(translation * -delta));
		
#if !(RN_PLATFORM_IOS)
		_finalcam->SetPosition(_camera->Position());
		_finalcam->SetRotation(_camera->Rotation());
#endif
	}
	
	
	void World::CreateSSAOStage()
	{
		// Surface normals + depth stage
		RN::Shader *surfaceShader = RN::Shader::WithFile("shader/SurfaceNormals");
		RN::Material *surfaceMaterial = new RN::Material(surfaceShader);
		
		_camera->SetMaterial(surfaceMaterial);
		
		// SSAO stage
		RN::Texture *noise = RN::Texture::WithFile("textures/SSAO_noise.png", RN::Texture::FormatRGB888, RN::Texture::WrapModeRepeat, RN::Texture::FilterLinear, true);
		RN::Shader *ssao = RN::Shader::WithFile("shader/SSAO");
		
		RN::Material *ssaoMaterial = new RN::Material(ssao);
		ssaoMaterial->AddTexture(noise);
		
		RN::Camera *ssaoStage  = new RN::Camera(RN::Vector2(), RN::Texture::FormatR8, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget, RN::RenderStorage::BufferFormatColor);
		ssaoStage->SetMaterial(ssaoMaterial);
		ssaoMaterial->Release();
		
		_camera->AddStage(ssaoStage);
		
		// Blur
		RN::Shader *blurVertical = new RN::Shader();
		RN::Shader *blurHorizontal = new RN::Shader();
		
		blurVertical->SetVertexShader("shader/GaussianVertical.vsh");
		blurVertical->SetFragmentShader("shader/Gaussian.fsh");
		blurVertical->Link();
		
		blurHorizontal->SetVertexShader("shader/GaussianHorizontal.vsh");
		blurHorizontal->SetFragmentShader("shader/Gaussian.fsh");
		blurHorizontal->Link();
		
		RN::Material *verticalMaterial = new RN::Material(blurVertical->Autorelease<RN::Shader>());
		RN::Material *horizontalMateral = new RN::Material(blurHorizontal->Autorelease<RN::Shader>());
		
		RN::Camera *verticalStage  = new RN::Camera(RN::Vector2(), RN::Texture::FormatR8, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget);
		RN::Camera *horizontalStage  = new RN::Camera(RN::Vector2(), RN::Texture::FormatR8, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget);
		
		verticalStage->SetMaterial(verticalMaterial);		
		horizontalStage->SetMaterial(horizontalMateral);
		 
		_camera->AddStage(verticalStage);
		_camera->AddStage(horizontalStage);
		
		// World stage
		RN::Camera *sceneCamera = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGB888, RN::Camera::FlagInherit);
		_camera->AddStage(sceneCamera);
		
		// SSAO post stage
		RN::Shader *ssaoPost = new RN::Shader();
		ssaoPost->SetVertexShader("shader/SSAO.vsh");
		ssaoPost->SetFragmentShader("shader/SSAOPost.fsh");
		ssaoPost->Link();
		
		RN::Material *ssaoPostMaterial = new RN::Material(ssaoPost);
		ssaoPostMaterial->AddTexture(horizontalStage->RenderTarget());
		
		RN::Camera *ssaoPostStage = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGB888, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget);
		ssaoPostStage->SetMaterial(ssaoPostMaterial);
		
		_camera->AddStage(ssaoPostStage);
	}
	
	void World::CreateWorld()
	{
		// Blocks
/*		RN::Texture *blockTexture0 = RN::Texture::WithFile("textures/brick.png", RN::Texture::FormatRGB888);
		RN::Texture *blockTexture1 = RN::Texture::WithFile("textures/testpng.png", RN::Texture::FormatRGB565);
		
		RN::Material *blockMaterial = new RN::Material(0);
		blockMaterial->AddTexture(blockTexture0);
		blockMaterial->AddTexture(blockTexture1);
		
		RN::Mesh  *blockMesh = RN::Mesh::CubeMesh(RN::Vector3(0.5f, 0.5f, 0.5f));
		RN::Model *blockModel = RN::Model::WithMesh(blockMesh->Autorelease<RN::Mesh>(), blockMaterial->Autorelease<RN::Material>());
		
		_block1 = new RN::RigidBodyEntity();
		_block1->SetModel(blockModel);
		_block1->SetPosition(RN::Vector3(-1.0f, 8.0f, -8.0f));
		_block1->SetSize(RN::Vector3(0.5f));
		_block1->SetMass(10.0f);
		_block1->SetRestitution(0.8);
		_block1->SetFriction(1.8f);
		_block1->ApplyImpulse(RN::Vector3(0.0f, -100.0f, 0.0f));
		
		_block2 = new RN::RigidBodyEntity();
		_block2->SetModel(blockModel);
		_block2->SetRotation(RN::Quaternion(RN::Vector3(45.0f)));
		_block2->SetPosition(RN::Vector3(1.0f, 8.0f, -8.0f));
		_block2->SetSize(RN::Vector3(0.5f));
		_block2->SetMass(10.0f);
		
		_block3 = new RN::RigidBodyEntity();
		_block3->SetModel(blockModel);
		_block3->SetRotation(RN::Quaternion(RN::Vector3(45.0f)));
		_block3->SetPosition(RN::Vector3(0.0f, 8.0f, -10.0f));
		_block3->SetSize(RN::Vector3(0.5f));
		_block3->SetMass(10.0f);
		_block3->ApplyImpulse(RN::Vector3(0.0f, 0.0f, -15.0f));
		
		// Floor
		RN::Shader *floorShader = RN::Shader::WithFile("shader/Ground");
		RN::Texture *floorTexture = RN::Texture::WithFile("textures/tiles.png", RN::Texture::FormatRGB888);
		
		RN::Material *floorMaterial = new RN::Material(floorShader);
		floorMaterial->AddTexture(floorTexture);
		
		RN::Mesh *floorMesh = RN::Mesh::CubeMesh(RN::Vector3(5.0f, 0.5f, 5.0f));
		RN::Model *floorModel = new RN::Model(floorMesh->Autorelease<RN::Mesh>(), floorMaterial->Autorelease<RN::Material>());
		
		_floor = new RN::RigidBodyEntity();
		_floor->SetModel(floorModel);
		_floor->SetPosition(RN::Vector3(0.0f, -5.0f, -8.0f));
		_floor->SetMass(0.0f);
		_floor->SetSize(RN::Vector3(5.0f, 0.5f, 5.0f));
		_floor->SetRestitution(0.5f);*/
		
		RN::Shader *shader = RN::Shader::WithFile("shader/rn_Texture1DiscardLight");
		RN::Shader *lightshader = RN::Shader::WithFile("shader/rn_Texture1Light");
		
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		for(int i = 0; i < model->Meshes(); i++)
		{
			model->MaterialForMesh(model->MeshAtIndex(i))->SetShader(lightshader);
		}
		model->MaterialForMesh(model->MeshAtIndex(4))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(4))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(10))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(10))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(19))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(19))->culling = false;
		
		RN::Entity *sponza = new RN::Entity();
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.1, 0.1, 0.1));
		sponza->Rotate(RN::Vector3(0.0, 0.0, -90.0));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
		RN::LightEntity *light;
		
		light = new RN::LightEntity();
		light->SetPosition(RN::Vector3(-30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Vector3((float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX));
		
		light = new RN::LightEntity();
		light->SetPosition(RN::Vector3(30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Vector3((float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX));
		
		for(int i = 0; i < 400; i++)
		{
			light = new RN::LightEntity();
			light->SetPosition(RN::Vector3((float)(rand())/RAND_MAX*280.0f-140.0f, (float)(rand())/RAND_MAX*100.0f, (float)(rand())/RAND_MAX*120.0f-50.0f));
			light->SetRange((float)(rand())/RAND_MAX*20.0f);
			light->SetColor(RN::Vector3((float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX, (float)(rand())/RAND_MAX));
		}
		
#if RN_TARGET_OPENGL
/*		RN::Shader *instancedShader = RN::Shader::WithFile("shader/rn_Texture1DiscardLight_instanced");
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
		
		for(float x = -100.0f; x < 200.0f; x += 1.0f)
		{
			for(float y = -10.0f; y < 10.0f; y += 1.0f)
			{
				index = (index + 1) % 4;
				
				RN::Entity *fern = new RN::Entity();
				fern->SetModel(foliage[index]);
				fern->Rotate(RN::Vector3(0.0, 0.0, -90.0));
				fern->SetPosition(RN::Vector3(x, -5.3, y));
			}
		}*/
#endif
	}
}
