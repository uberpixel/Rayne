//
//  TGWorld.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGWorld.h"

#define TGWorldFeatureLights        1
#define TGWorldFeatureNormalMapping 0
#define TGWorldFeatureInstancing    0
#define TGWorldFeatureFreeCamera    1
#define TGWorldFeatureParticles     1

#define TGWorldRandom (float)(rand())/RAND_MAX
#define TGWorldSpotLightRange 95.0f

namespace TG
{
	World::World() :
		RN::World("GenericSceneManager")
	{
		_physicsAttachment = new RN::bullet::PhysicsWorld();
		AddAttachment(_physicsAttachment->Autorelease());
		
		CreateCameras();
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
		
		static bool fpressed = false;
		if(input->KeyPressed('f'))
		{
			if(!fpressed)
			{
				_spotLight->SetRange(_spotLight->Range() > 1.0f ? 0.0f : TGWorldSpotLightRange);
				fpressed = true;
			}
		}
		else
		{
			fpressed = false;
		}
		
#if TGWorldFeatureFreeCamera
		RN::Vector3 translation;
		RN::Vector3 rotation;
		
		const RN::Vector3& mouseDelta = input->MouseDelta() * -0.2f;
		
		rotation.x = mouseDelta.x;
		rotation.z = mouseDelta.y;
		
		translation.x = (input->KeyPressed('d') - input->KeyPressed('a')) * 16.0f;
		translation.z = (input->KeyPressed('s') - input->KeyPressed('w')) * 16.0f;
		
		_camera->Rotate(rotation);
		_camera->TranslateLocal(translation * delta);
#endif
	}
	
	void World::CreateCameras()
	{
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatDepth | RN::RenderStorage::BufferFormatStencil);
		RN::Texture *depthtex = new RN::Texture(RN::Texture::FormatDepthStencil);
		storage->SetDepthTarget(depthtex);
		
		RN::Shader *depthShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightDepthShader);
		RN::Material *depthMaterial = new RN::Material(depthShader);
		
		_camera = new ThirdPersonCamera(storage);
		_camera->SetMaterial(depthMaterial);
		
		RN::Shader *downsampleShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightTileSampleShader);
		RN::Shader *downsampleFirstShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightTileSampleFirstShader);
		
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
		_finalcam->renderGroup |= RN::Camera::RenderGroup1;
		
		if(RN::Kernel::SharedInstance()->ScaleFactor() == 2.0f)
		{
			_finalcam->ActivateTiledLightLists(downsample64x->Storage()->RenderTarget());
		}
		else
		{
			_finalcam->ActivateTiledLightLists(downsample32x->Storage()->RenderTarget());
		}
		
		_camera->AttachChild(_finalcam);
	}
	
	void World::CreateWorld()
	{
		// Sponza
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		model->MaterialAtIndex(0, 5)->discard = true;
		model->MaterialAtIndex(0, 5)->culling = false;
		model->MaterialAtIndex(0, 5)->alphatest = true;
		model->MaterialAtIndex(0, 5)->override = RN::Material::OverrideGroupDiscard;
		
		model->MaterialAtIndex(0, 6)->discard = true;
		model->MaterialAtIndex(0, 6)->culling = false;
		model->MaterialAtIndex(0, 6)->alphatest = true;
		model->MaterialAtIndex(0, 6)->override = RN::Material::OverrideGroupDiscard;
		
		model->MaterialAtIndex(0, 17)->discard = true;
		model->MaterialAtIndex(0, 17)->culling = false;
		model->MaterialAtIndex(0, 17)->alphatest = true;
		model->MaterialAtIndex(0, 17)->override = RN::Material::OverrideGroupDiscard;
		
#if TGWorldFeatureNormalMapping && TGWorldFeatureLights
		RN::Shader *normalshader = RN::Shader::WithFile("shader/rn_Texture1Normal");
		RN::Texture *normalmap = RN::Texture::WithFile("models/sponza/spnza_bricks_a_ddn.png", RN::Texture::FormatRGBA8888);
		
		model->MaterialAtIndex(0, 3)->AddTexture(normalmap);
		model->MaterialAtIndex(0, 3)->SetShader(normalshader);
#endif
		
		RN::bullet::Shape *sponzaShape = new RN::bullet::TriangelMeshShape(model);
		RN::bullet::PhysicsMaterial *sponzaMaterial = new RN::bullet::PhysicsMaterial();
		
		sponzaShape->SetScale(RN::Vector3(0.5));
		
		RN::bullet::RigidBody *sponza = new RN::bullet::RigidBody(sponzaShape, 0.0f);
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.5, 0.5, 0.5));
		sponza->SetMaterial(sponzaMaterial);
		sponza->SetRotation(RN::Quaternion(RN::Vector3(0.0, 0.0, -90.0)));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
#if !TGWorldFeatureFreeCamera
		RN::Model *playerModel = RN::Model::WithFile("models/TiZeta/simplegirl.sgm");
		RN::Skeleton *playerSkeleton = RN::Skeleton::WithFile("models/TiZeta/simplegirl.sga");
		playerSkeleton->SetAnimation("cammina");
		
		_player = new Player(playerModel);
		_player->SetSkeleton(playerSkeleton);
		_player->SetPosition(RN::Vector3(5.0f, -5.0f, 0.0f));
		_player->SetCamera(_camera);
		
		_camera->SetTarget(_player);
#endif
				
#if TGWorldFeatureParticles
		RN::Texture *texture = RN::Texture::WithFile("textures/particle.png", RN::Texture::FormatRGBA8888);
		
		RN::ParticleMaterial *material = new RN::ParticleMaterial();
		material->AddTexture(texture);
		
		material->lifespan = 10.0f;
		material->minVelocity = RN::Vector3(0.0f, 0.5f, 0.0f);
		material->maxVelocity = RN::Vector3(0.0f, 0.15f, 0.0f);
		
		material->discard = true;
		material->blending = true;
		material->SetBlendMode(RN::Material::BlendMode::Interpolative);
		
		DustEmitter *emitter = new DustEmitter();
		emitter->SetMaterial(material);
		emitter->Cook(100.0f, 10);
		emitter->group = 1;
#endif
		
#if TGWorldFeatureLights
		RN::Light *light;
		//srand(time(0));
		
		light = new RN::Light();
		light->SetPosition(RN::Vector3(-30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		
		light = new RN::Light();
		light->SetPosition(RN::Vector3(30.0f, 0.0f, 0.0f));
		light->SetRange(80.0f);
		light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		
		//light = new RN::Light(RN::Light::TypeDirectionalLight);
		//light->SetRotation(RN::Quaternion(RN::Vector3(60.0f, -60.0f, 60.0f)));
		
		_spotLight = new RN::Light(RN::Light::TypeSpotLight);
		_spotLight->SetPosition(RN::Vector3(0.75f, -0.5f, 0.0f));
		_spotLight->SetRange(TGWorldSpotLightRange);
		_spotLight->SetAngle(0.9f);
		_spotLight->SetColor(RN::Color(0.5f));
		
#if TGWorldFeatureFreeCamera
		_camera->AttachChild(_spotLight);
#else
		_player->AttachChild(_spotLight);
#endif
		
		for(int i=0; i<200; i++)
		{
			light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 140.0f - 70.0f, TGWorldRandom * 100.0f-20.0f, TGWorldRandom * 80.0f - 40.0f));
			light->SetRange((TGWorldRandom * 20.0f) + 10.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
			
			/*light->SetAction([](RN::Transform *transform, float delta) {
				transform->Translate(RN::Vector3(0.5f * delta, 0.0f, 0.0));
			});*/
		}
#endif
		
		RN::Billboard *billboard = new RN::Billboard();
		
		billboard->SetTexture(RN::Texture::WithFile("textures/billboard.png", RN::Texture::FormatRGBA8888));
		billboard->SetScale(RN::Vector3(0.2f));
		billboard->SetRotation(RN::Quaternion(RN::Vector3(90.0f, 0.0f, 0.0f)));
		//billboard->TranslateLocal(RN::Vector3(0.0f, 9.0f, 57.0f));
		
#if TGWorldFeatureInstancing
		RN::Model *foliage[4];
		
		foliage[0] = RN::Model::WithFile("models/nobiax/fern_01.sgm");
		foliage[0]->MaterialAtIndex(0, 0)->culling = false;
		foliage[0]->MaterialAtIndex(0, 0)->discard = true;
		foliage[0]->MaterialAtIndex(0, 0)->alphatest = true;
	
		foliage[1] = RN::Model::WithFile("models/nobiax/grass_05.sgm");
		foliage[1]->MaterialAtIndex(0, 0)->culling = false;
		foliage[1]->MaterialAtIndex(0, 0)->discard = true;
		foliage[1]->MaterialAtIndex(0, 0)->alphatest = true;
		
		foliage[2] = RN::Model::WithFile("models/nobiax/grass_19.sgm");
		foliage[2]->MaterialAtIndex(0, 0)->culling = false;
		foliage[2]->MaterialAtIndex(0, 0)->discard = true;
		foliage[2]->MaterialAtIndex(0, 0)->alphatest = true;
		
		foliage[3] = RN::Model::WithFile("models/nobiax/grass_04.sgm");
		foliage[3]->MaterialAtIndex(0, 0)->culling = false;
		foliage[3]->MaterialAtIndex(0, 0)->discard = true;
		foliage[3]->MaterialAtIndex(0, 0)->alphatest = true;
		
		uint32 index = 0;
		
		for(float x = -100.0f; x < 200.0f; x += 1.5f)
		{
			for(float y = -10.0f; y < 10.0f; y += 1.0f)
			{
				index = (index + 1) % 4;
				
				RN::Entity *fern = new RN::Entity();
				fern->SetModel(foliage[index]);
				fern->Rotate(RN::Vector3(0.0, 0.0, -90.0));
				fern->SetPosition(RN::Vector3(x, -13.3, y));
			}
		}
#endif
	}
}
