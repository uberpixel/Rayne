//
//  TGWorld.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGWorld.h"

#define TGWorldFeatureLights        1
#define TGWorldFeatureNormalMapping 1
#define TGWorldFeatureInstancing    0
#define TGWorldFeatureAnimations    0

#define TGWorldRandom (float)(rand())/RAND_MAX
#define TGWorldSpotLightRange 95.0f

namespace TG
{
	World::World()
	{
#if !(RN_PLATFORM_IOS)
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatDepth | RN::RenderStorage::BufferFormatStencil);
		RN::Texture *depthtex = new RN::Texture(RN::Texture::FormatDepthStencil);
		storage->SetDepthTarget(depthtex);
		
		RN::Shader *depthShader = RN::Shader::WithFile("shader/rn_LightDepth");
		RN::Material *depthMaterial = new RN::Material(depthShader);
		
		_camera = new RN::Camera(RN::Vector2(), storage, RN::Camera::FlagDefaults);
		_camera->SetMaterial(depthMaterial);
		
		RN::Shader *downsampleShader = RN::Shader::WithFile("shader/rn_LightTileSample");
		RN::Shader *downsampleFirstShader = RN::Shader::WithFile("shader/rn_LightTileSampleFirst");
		RN::Material *downsampleMaterial2x = new RN::Material(downsampleFirstShader);
		downsampleMaterial2x->AddTexture(depthtex);

		RN::Camera *downsample2x = new RN::Camera(RN::Vector2(_camera->Frame().width/2, _camera->Frame().height/2), RN::Texture::FormatRG32F, RN::Camera::FlagUpdateStorageFrame|RN::Camera::FlagDrawTarget|RN::Camera::FlagInheritProjection, RN::RenderStorage::BufferFormatComplete);
		_camera->AddStage(downsample2x);
		downsample2x->SetMaterial(downsampleMaterial2x);

		RN::Material *downsampleMaterial4x = new RN::Material(downsampleShader);
		downsampleMaterial4x->AddTexture(downsample2x->Storage()->RenderTarget());

		RN::Camera *downsample4x = new RN::Camera(RN::Vector2(_camera->Frame().width/4, _camera->Frame().height/4), RN::Texture::FormatRG32F, RN::Camera::FlagUpdateStorageFrame|RN::Camera::FlagDrawTarget|RN::Camera::FlagInheritProjection, RN::RenderStorage::BufferFormatColor);
		_camera->AddStage(downsample4x);
		downsample4x->SetMaterial(downsampleMaterial4x);

		RN::Material *downsampleMaterial8x = new RN::Material(downsampleShader);
		downsampleMaterial8x->AddTexture(downsample4x->Storage()->RenderTarget());

		RN::Camera *downsample8x = new RN::Camera(RN::Vector2(_camera->Frame().width/8, _camera->Frame().height/8), RN::Texture::FormatRG32F, RN::Camera::FlagUpdateStorageFrame|RN::Camera::FlagDrawTarget|RN::Camera::FlagInheritProjection, RN::RenderStorage::BufferFormatColor);
		_camera->AddStage(downsample8x);
		downsample8x->SetMaterial(downsampleMaterial8x);

		RN::Material *downsampleMaterial16x = new RN::Material(downsampleShader);
		downsampleMaterial16x->AddTexture(downsample8x->Storage()->RenderTarget());

		RN::Camera *downsample16x = new RN::Camera(RN::Vector2(_camera->Frame().width/16, _camera->Frame().height/16), RN::Texture::FormatRG32F, RN::Camera::FlagUpdateStorageFrame|RN::Camera::FlagDrawTarget|RN::Camera::FlagInheritProjection, RN::RenderStorage::BufferFormatColor);
		_camera->AddStage(downsample16x);
		downsample16x->SetMaterial(downsampleMaterial16x);

		RN::Material *downsampleMaterial32x = new RN::Material(downsampleShader);
		downsampleMaterial32x->AddTexture(downsample16x->Storage()->RenderTarget());

		RN::Camera *downsample32x = new RN::Camera(RN::Vector2(_camera->Frame().width/32, _camera->Frame().height/32), RN::Texture::FormatRG32F, RN::Camera::FlagUpdateStorageFrame|RN::Camera::FlagDrawTarget|RN::Camera::FlagInheritProjection, RN::RenderStorage::BufferFormatColor);
		_camera->AddStage(downsample32x);
		downsample32x->SetMaterial(downsampleMaterial32x);

		RN::Camera *downsample64x;
		if(RN::Kernel::SharedInstance()->ScaleFactor() == 2.0f)
		{
			RN::Material *downsampleMaterial64x = new RN::Material(downsampleShader);
			downsampleMaterial64x->AddTexture(downsample32x->Storage()->RenderTarget());

			downsample64x = new RN::Camera(RN::Vector2(_camera->Frame().width/64, _camera->Frame().height/64), RN::Texture::FormatRG32F, RN::Camera::FlagUpdateStorageFrame|RN::Camera::FlagDrawTarget|RN::Camera::FlagInheritProjection, RN::RenderStorage::BufferFormatColor);
			_camera->AddStage(downsample64x);
			downsample64x->SetMaterial(downsampleMaterial64x);
		}

		_finalcam = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGBA32F, RN::Camera::FlagDefaults);
		_finalcam->SetClearMask(RN::Camera::ClearFlagColor);
		_finalcam->Storage()->SetDepthTarget(depthtex);
		_finalcam->SetSkyCube(RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png"));
//		_camera->AttachChild(_finalcam);

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
		
		
		
		
/*		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatColor | RN::RenderStorage::BufferFormatDepth | RN::RenderStorage::BufferFormatStencil);
		RN::Texture *depthtex = new RN::Texture(RN::Texture::FormatDepthStencil);
		
		storage->SetDepthTarget(depthtex);
		storage->AddRenderTarget(RN::Texture::FormatRGBA8888);
		
		_camera = new RN::Camera(RN::Vector2(), storage, RN::Camera::FlagDefaults);
		_camera->SetClearColor(RN::Color(0.0, 0.0, 0.0, 1.0));
		_camera->ActivateTiledLightLists((RN::Texture *)1);
		_camera->SetSkyCube(RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png"));
		*/
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
		
#if TGWorldFeatureLights
		static bool fpressed = false;
		
		if(input->KeyPressed('f'))
		{
			if(!fpressed)
			{
				fpressed = true;
				
				RN::LightEntity *child = _camera->ChildAtIndex<RN::LightEntity>(0);
				child->SetRange((child->Range() < 1.0f) ? TGWorldSpotLightRange : 0.0f);
			}
		}
		else
		{
			fpressed = false;
		}
#endif
		
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
		
#if TGWorldFeatureAnimations
		_girlskeleton->Update(delta*24.0f);
		_zombieskeleton->Update(delta*24.0f);
#endif
		
		_camera->Rotate(rotation);
		_camera->Translate(translation * -delta, true);
		
		_finalcam->SetRotation(_camera->Rotation());
		_finalcam->SetPosition(_camera->Position());
	}
	
	void World::CreateWorld()
	{
		RN::Shader *discardShader = RN::Shader::WithFile("shader/rn_Texture1Discard");
		RN::Shader *shader = RN::Shader::WithFile("shader/rn_Texture1");
		
		// Sponza
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		for(int i = 0; i < model->Meshes(); i++)
		{
			model->MaterialForMesh(model->MeshAtIndex(i))->SetShader(shader);
		}
		model->MaterialForMesh(model->MeshAtIndex(5))->SetShader(discardShader);
		model->MaterialForMesh(model->MeshAtIndex(5))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(5))->alphatest = true;
		model->MaterialForMesh(model->MeshAtIndex(6))->SetShader(discardShader);
		model->MaterialForMesh(model->MeshAtIndex(6))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(6))->alphatest = true;
		model->MaterialForMesh(model->MeshAtIndex(17))->SetShader(discardShader);
		model->MaterialForMesh(model->MeshAtIndex(17))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(17))->alphatest = true;
		
#if TGWorldFeatureNormalMapping && TGWorldFeatureLights
		RN::Shader *normalshader = RN::Shader::WithFile("shader/rn_Texture1Normal");
		RN::Texture *normalmap = RN::Texture::WithFile("models/sponza/spnza_bricks_a_ddn.png", RN::Texture::FormatRGBA8888);
		model->MaterialForMesh(model->MeshAtIndex(3))->AddTexture(normalmap);
		model->MaterialForMesh(model->MeshAtIndex(3))->SetShader(normalshader);
#endif
		
		RN::Entity *sponza = new RN::Entity();
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.5, 0.5, 0.5));
		sponza->Rotate(RN::Vector3(0.0, 0.0, -90.0));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
		
		// Blocks
		RN::Material *blockMaterial = new RN::Material(shader);
		blockMaterial->AddTexture(RN::Texture::WithFile("textures/brick.png", RN::Texture::FormatRGB888));
		
		RN::Mesh  *blockMesh = RN::Mesh::CubeMesh(RN::Vector3(0.5f, 0.5f, 0.5f));
		RN::Model *blockModel = RN::Model::WithMesh(blockMesh->Autorelease(), blockMaterial->Autorelease());
		
		_parentBlock = new RN::Entity();
		_parentBlock->SetModel(blockModel);
		
		_childBlock = new RN::Entity();
		_childBlock->Autorelease();
		_childBlock->SetModel(blockModel);
		_childBlock->SetAction([](RN::Entity *entity, float delta) {
			entity->Rotate(RN::Vector3(32.0f * delta, 0.0f, 32.0f * delta));
			entity->SetPosition(RN::Vector3(0.0f, 2.0f + (sinf(RN::Kernel::SharedInstance()->Time())), 0.0f));
			
			if(RN::Kernel::SharedInstance()->Time() > 5.0f)
				entity->DetachFromParent();
		});
		
		_parentBlock->AttachChild(_childBlock);
		_parentBlock->SetAction([](RN::Entity *entity, float delta) {
			entity->Rotate(RN::Vector3(0.0f, 64.0f * delta, 0.0f));
		});
		
#if TGWorldFeatureAnimations
		RN::Model *girlmodel = RN::Model::WithFile("models/TiZeta/simplegirl.sgm");
		_girlskeleton = RN::Skeleton::WithFile("models/TiZeta/simplegirl.sga");
		_girlskeleton->SetAnimation("cammina");
		 
		RN::Entity *girl = new RN::Entity();
		girl->SetModel(girlmodel);
		girl->SetSkeleton(_girlskeleton);
		girl->SetPosition(RN::Vector3(5.0f, -5.0f, 0.0f));
		girlmodel->MaterialForMesh(girlmodel->MeshAtIndex(0))->SetShader(shader);
		
		
		RN::Model *zombiemodel = RN::Model::WithFile("models/RosswetMobile/new_thin_zombie.sgm");
		_zombieskeleton = RN::Skeleton::WithFile("models/RosswetMobile/new_thin_zombie.sga");
		_zombieskeleton->SetAnimation("idle");
		 
		RN::Entity *zombie = new RN::Entity();
		zombie->SetModel(zombiemodel);
		zombie->SetSkeleton(_zombieskeleton);
		zombie->SetPosition(RN::Vector3(-5.0f, -5.0f, 0.0f));
		zombiemodel->MaterialForMesh(zombiemodel->MeshAtIndex(0))->SetShader(shader);
#endif
		
		RN::Shader *foliageShader = new RN::Shader();
		foliageShader->SetVertexShader("shader/rn_WindFoliage.vsh");
		foliageShader->SetFragmentShader("shader/rn_Texture1Discard.fsh");
		
		RN::Model *spruceModel = RN::Model::WithFile("models/dexfuck/spruce2.sgm");
		
		spruceModel->MaterialForMesh(spruceModel->MeshAtIndex(0))->SetShader(foliageShader);
		spruceModel->MaterialForMesh(spruceModel->MeshAtIndex(0))->culling = false;
		spruceModel->MaterialForMesh(spruceModel->MeshAtIndex(0))->alphatest = true;
		
		spruceModel->MaterialForMesh(spruceModel->MeshAtIndex(1))->SetShader(foliageShader);
		spruceModel->MaterialForMesh(spruceModel->MeshAtIndex(1))->culling = false;
		spruceModel->MaterialForMesh(spruceModel->MeshAtIndex(1))->alphatest = true;
		
		_spruce = new RN::Entity();
		_spruce->SetModel(spruceModel);
		
#if TGWorldFeatureLights
		RN::LightEntity *light;
		
		//srand(time(0));
		
		light = new RN::LightEntity();
		light->SetPosition(RN::Vector3(-30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Vector3(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		
		light = new RN::LightEntity();
		light->SetPosition(RN::Vector3(30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Vector3(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		
		light = new RN::LightEntity(RN::LightEntity::TypeSpotLight);
		light->SetPosition(RN::Vector3(0.75f, -0.5f, 0.0f));
		light->SetRange(TGWorldSpotLightRange);
		light->SetAngle(0.9f);
		light->SetColor(RN::Vector3(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		_camera->AttachChild(light);
		
		for(int i=0; i<100; i++)
		{
			light = new RN::LightEntity();
			light->SetPosition(RN::Vector3(TGWorldRandom * 280.0f - 140.0f, TGWorldRandom * 100.0f, TGWorldRandom * 120.0f - 50.0f));
			light->SetRange(TGWorldRandom * 20.0f);
			light->SetColor(RN::Vector3(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		}
#endif
		
#if TGWorldFeatureInstancing
		RN::Model *foliage[4];
		
		foliage[0] = RN::Model::WithFile("models/nobiax/fern_01.sgm");
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->SetShader(foliageShader);
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->culling = false;
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->alphatest = true;
		
		foliage[1] = RN::Model::WithFile("models/nobiax/grass_05.sgm");
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->SetShader(foliageShader);
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->culling = false;
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->alphatest = true;
		
		foliage[2] = RN::Model::WithFile("models/nobiax/grass_19.sgm");
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->SetShader(foliageShader);
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->culling = false;
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->alphatest = true;
		
		foliage[3] = RN::Model::WithFile("models/nobiax/grass_04.sgm");
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->SetShader(foliageShader);
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->culling = false;
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->alphatest = true;
		
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
