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
#define TGWorldFeatureFreeCamera    1
#define TGWorldFeatureZPrePass		1
#define TGWorldFeatureBloom			1
#define TGWorldFeatureSSAO          0

#define TGWorldFeatureParticles     1
#define TGForestFeatureTrees 500
#define TGForestFeatureGras  10000

#define TGWorldRandom (float)(rand())/RAND_MAX
#define TGWorldSpotLightRange 95.0f

namespace TG
{
	World::World() :
		RN::World("GenericSceneManager")
	{
//		srand(time(0));
		
		_sunLight = 0;
		_finalcam = 0;
		_camera = 0;
		_spotLight = 0;
		_player = 0;
		
		_physicsAttachment = new RN::bullet::PhysicsWorld();
		AddAttachment(_physicsAttachment->Autorelease());
		
		CreateCameras();
		CreateWorld();
//		CreateForest();
		
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
				if(_spotLight)
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
		
		if(_sunLight != 0)
		{
			RN::Vector3 sunrot;
			sunrot.x = (input->KeyPressed('e') - input->KeyPressed('q')) * 5.0f;
			sunrot.z = (input->KeyPressed('r') - input->KeyPressed('t')) * 2.0f;
			_sunLight->Rotate(sunrot);
		}
	}
	
	void World::CreateCameras()
	{
#if TGWorldFeatureZPrePass
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatDepth|RN::RenderStorage::BufferFormatStencil);
		
		RN::TextureParameter depthparam;
		depthparam.format = RN::TextureParameter::Format::DepthStencil;
		depthparam.generateMipMaps = false;
		depthparam.mipMaps = 0;
		depthparam.wrapMode = RN::TextureParameter::WrapMode::Clamp;
		
		RN::Texture *depthtex = new RN::Texture(depthparam);
		storage->SetDepthTarget(depthtex);
		
		RN::Shader *depthShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightDepthShader);
		RN::Material *depthMaterial = new RN::Material(depthShader);
		
		_camera = new ThirdPersonCamera(storage);
		_camera->SetMaterial(depthMaterial);
		
		RN::PostProcessingPipeline *pipeline = _camera->AddPostProcessingPipeline("Downsample");
		
		RN::Shader *downsampleShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightTileSampleShader);
		RN::Shader *downsampleFirstShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightTileSampleFirstShader);
		
		uint32 flags = RN::Camera::FlagUpdateStorageFrame | RN::Camera::FlagInheritProjection;
		
		// 2x
		RN::Material *downsampleMaterial2x = new RN::Material(downsampleFirstShader);
		downsampleMaterial2x->AddTexture(depthtex);
		
		RN::Camera *downsample2x = new RN::Camera(_camera->Frame().Size() / 2.0f, RN::TextureParameter::Format::RG32F, flags, RN::RenderStorage::BufferFormatColor);
		downsample2x->SetMaterial(downsampleMaterial2x);
		
		pipeline->AddStage(downsample2x, RN::RenderStage::Mode::ReUsePreviousStage);
		
		// 4x
		RN::Material *downsampleMaterial4x = new RN::Material(downsampleShader);
		downsampleMaterial4x->AddTexture(downsample2x->Storage()->RenderTarget());
		
		RN::Camera *downsample4x = new RN::Camera(_camera->Frame().Size() / 4.0f, RN::TextureParameter::Format::RG32F, flags, RN::RenderStorage::BufferFormatColor);
		downsample4x->SetMaterial(downsampleMaterial4x);
		
		pipeline->AddStage(downsample4x, RN::RenderStage::Mode::ReUsePreviousStage);
		
		// 8x
		RN::Material *downsampleMaterial8x = new RN::Material(downsampleShader);
		downsampleMaterial8x->AddTexture(downsample4x->Storage()->RenderTarget());
		
		RN::Camera *downsample8x = new RN::Camera(_camera->Frame().Size() / 8.0f, RN::TextureParameter::Format::RG32F, flags, RN::RenderStorage::BufferFormatColor);
		downsample8x->SetMaterial(downsampleMaterial8x);
		
		pipeline->AddStage(downsample8x, RN::RenderStage::Mode::ReUsePreviousStage);
		
		// 16x
		RN::Material *downsampleMaterial16x = new RN::Material(downsampleShader);
		downsampleMaterial16x->AddTexture(downsample8x->Storage()->RenderTarget());
		
		RN::Camera *downsample16x = new RN::Camera(_camera->Frame().Size() / 16.0f, RN::TextureParameter::Format::RG32F, flags, RN::RenderStorage::BufferFormatColor);
		downsample16x->SetMaterial(downsampleMaterial16x);
		
		pipeline->AddStage(downsample16x, RN::RenderStage::Mode::ReUsePreviousStage);
		
		// 32x
		RN::Material *downsampleMaterial32x = new RN::Material(downsampleShader);
		downsampleMaterial32x->AddTexture(downsample16x->Storage()->RenderTarget());
		
		RN::Camera *downsample32x = new RN::Camera(_camera->Frame().Size() / 32.0f, RN::TextureParameter::Format::RG32F, flags, RN::RenderStorage::BufferFormatColor);
		downsample32x->SetMaterial(downsampleMaterial32x);
		
		pipeline->AddStage(downsample32x, RN::RenderStage::Mode::ReUsePreviousStage);
		
		// 64x
		RN::Camera *downsample64x;
		if(RN::Kernel::SharedInstance()->ScaleFactor() == 2.0f)
		{
			RN::Material *downsampleMaterial64x = new RN::Material(downsampleShader);
			downsampleMaterial64x->AddTexture(downsample32x->Storage()->RenderTarget());
			
			downsample64x = new RN::Camera(_camera->Frame().Size() / 64.0f, RN::TextureParameter::Format::RG32F, flags, RN::RenderStorage::BufferFormatColor);
			downsample64x->SetMaterial(downsampleMaterial64x);
			
			pipeline->AddStage(downsample64x, RN::RenderStage::Mode::ReUsePreviousStage);
		}

		_finalcam = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagDefaults);
		_finalcam->SetClearMask(RN::Camera::ClearFlagColor);
		_finalcam->Storage()->SetDepthTarget(depthtex);
		//_finalcam->SetSkyCube(RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png"));
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
		_camera->SetPriority(10);
		_camera->Rotate(RN::Vector3(90.0f, 0.0f, 0.0f));
		
		RN::Shader *combineShader = RN::Shader::WithFile("shader/rn_PPCombine");
		RN::Shader *blurShader = RN::Shader::WithFile("shader/rn_BoxBlur");
		RN::Shader *updownShader = RN::Shader::WithFile("shader/rn_PPCopy");
		
		RN::Material *blurXMaterial = new RN::Material(blurShader);
		blurXMaterial->Define("RN_BLURX");
		
		RN::Material *blurYMaterial = new RN::Material(blurShader);
		blurYMaterial->Define("RN_BLURY");
		
		RN::Material *updownMaterial = new RN::Material(updownShader);
		
		
#if TGWorldFeatureSSAO
		// Surface normals
		RN::Shader *surfaceShader = RN::Shader::WithFile("shader/rn_SurfaceNormals");
		RN::Material *surfaceMaterial = new RN::Material(surfaceShader);
		
		RN::Camera *normalsCamera = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatComplete);
		normalsCamera->SetMaterial(surfaceMaterial);
		normalsCamera->Storage()->SetDepthTarget(depthtex);
		normalsCamera->SetClearMask(RN::Camera::ClearFlagColor);
		
		// SSAO stage
		RN::Texture *ssaoNoise = RN::Texture::WithFile("textures/rn_SSAONoise.png");
		
		RN::Shader *ssaoShader = RN::Shader::WithFile("shader/rn_SSAO");
		RN::Material *ssaoMaterial = new RN::Material(ssaoShader);
		ssaoMaterial->AddTexture(ssaoNoise);
		
		RN::Camera *ssaoCamera  = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::R8, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoCamera->SetMaterial(ssaoMaterial);
		
		// Blur X
		RN::Camera *ssaoBlurX = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::R8, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoBlurX->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *ssaoBlurY = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::R8, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoBlurY->SetMaterial(blurYMaterial);
		
		// Combine stage
		RN::Material *ssaoCombineMaterial = new RN::Material(combineShader);
		ssaoCombineMaterial->AddTexture(ssaoBlurY->Storage()->RenderTarget());
		ssaoCombineMaterial->Define("MODE_GRAYSCALE");
		
		RN::Camera *ssaoCombineCamera  = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGB888, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoCombineCamera->SetMaterial(ssaoCombineMaterial);
		
		// PP pipeline
		RN::PostProcessingPipeline *ssaoPipeline = _finalcam->AddPostProcessingPipeline("SSAO");
		ssaoPipeline->AddStage(normalsCamera, RN::RenderStage::Mode::ReRender);
		ssaoPipeline->AddStage(ssaoCamera, RN::RenderStage::Mode::ReUsePreviousStage);
		ssaoPipeline->AddStage(ssaoBlurX, RN::RenderStage::Mode::ReUsePreviousStage);
		ssaoPipeline->AddStage(ssaoBlurY, RN::RenderStage::Mode::ReUsePreviousStage);
		ssaoPipeline->AddStage(ssaoCombineCamera, RN::RenderStage::Mode::ReUsePipeline);
#endif
		
#if TGWorldFeatureBloom
		// Filter bright
		RN::Shader *filterBrightShader = RN::Shader::WithFile("shader/rn_FilterBright");
		RN::Material *filterBrightMaterial = new RN::Material(filterBrightShader);
		RN::Camera *filterBright = new RN::Camera(_camera->Frame().Size() / 2.0f, RN::TextureParameter::Format::RGB888, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		filterBright->SetMaterial(filterBrightMaterial);
		
		// Down sample
		RN::Camera *downSample4x = new RN::Camera(_camera->Frame().Size() / 4.0f, RN::TextureParameter::Format::RGB888, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample4x->SetMaterial(updownMaterial);
		
		// Down sample
		RN::Camera *downSample8x = new RN::Camera(_camera->Frame().Size() / 8.0f, RN::TextureParameter::Format::RGB888, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample8x->SetMaterial(updownMaterial);
		
		// Down sample
		RN::Camera *downSample16x = new RN::Camera(_camera->Frame().Size() / 16.0f, RN::TextureParameter::Format::RGB888, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample16x->SetMaterial(updownMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXlow = new RN::Camera(_camera->Frame().Size() / 16.0f, RN::TextureParameter::Format::RGB888, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXlow->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYlow = new RN::Camera(_camera->Frame().Size() / 16.0f, RN::TextureParameter::Format::RGB888, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYlow->SetMaterial(blurYMaterial);
		
		// Up sample
		RN::Camera *upSample = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGB888, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		upSample->SetMaterial(updownMaterial);
		
		// Blur X
		RN::Camera *bloomBlurX = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGB888, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurX->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurY = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGB888, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurY->SetMaterial(blurYMaterial);
	
		// Combine
		RN::Material *bloomCombineMaterial = new RN::Material(combineShader);
		bloomCombineMaterial->AddTexture(bloomBlurY->Storage()->RenderTarget());
		
		RN::Camera *bloomCombine = new RN::Camera(RN::Vector2(0.0f), RN::TextureParameter::Format::RGB888, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomCombine->SetMaterial(bloomCombineMaterial);
		
		RN::PostProcessingPipeline *bloom = _finalcam->AddPostProcessingPipeline("Bloom");
		bloom->AddStage(filterBright, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample4x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample8x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample16x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurXlow, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurYlow, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(upSample, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurX, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurY, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomCombine, RN::RenderStage::Mode::ReUsePipeline);
#endif
		
#else
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatComplete);
		storage->AddRenderTarget(RN::TextureParameter::Format::RGBA32F);
		_camera = new ThirdPersonCamera(storage);
#endif
	}
	
	void World::CreateWorld()
	{
		// Sponza
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		model->MaterialAtIndex(0, 5)->discard = true;
		model->MaterialAtIndex(0, 5)->culling = false;
		model->MaterialAtIndex(0, 5)->override = RN::Material::OverrideGroupDiscard;
		
		model->MaterialAtIndex(0, 6)->discard = true;
		model->MaterialAtIndex(0, 6)->culling = false;
		model->MaterialAtIndex(0, 6)->override = RN::Material::OverrideGroupDiscard;
		
		model->MaterialAtIndex(0, 17)->discard = true;
		model->MaterialAtIndex(0, 17)->culling = false;
		model->MaterialAtIndex(0, 17)->override = RN::Material::OverrideGroupDiscard;
		
#if TGWorldFeatureNormalMapping && TGWorldFeatureLights
		model->MaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/sponza/lion_ddn.png"));
		model->MaterialAtIndex(0, 0)->Define("RN_NORMALMAP");
		
		model->MaterialAtIndex(0, 1)->AddTexture(RN::Texture::WithFile("models/sponza/background_ddn.png"));
		model->MaterialAtIndex(0, 1)->Define("RN_NORMALMAP");
		
		model->MaterialAtIndex(0, 2)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_c_ddn.png"));
		model->MaterialAtIndex(0, 2)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_c_spec.png"));
		model->MaterialAtIndex(0, 2)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 2)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 2)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 2)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 3)->AddTexture(RN::Texture::WithFile("models/sponza/spnza_bricks_a_ddn.png"));
		model->MaterialAtIndex(0, 3)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 3)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 3)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 4)->AddTexture(RN::Texture::WithFile("models/sponza/vase_ddn.png"));
		model->MaterialAtIndex(0, 4)->Define("RN_NORMALMAP");
		
		model->MaterialAtIndex(0, 5)->AddTexture(RN::Texture::WithFile("models/sponza/chain_texture_ddn.png"));
		model->MaterialAtIndex(0, 5)->specular = RN::Color(0.5f, 0.5f, 0.5f, 1.0f);
		model->MaterialAtIndex(0, 5)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 5)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 6)->AddTexture(RN::Texture::WithFile("models/sponza/vase_plant_spec.png"));
		model->MaterialAtIndex(0, 6)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 6)->Define("RN_SPECMAP");
		model->MaterialAtIndex(0, 6)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 7)->AddTexture(RN::Texture::WithFile("models/sponza/vase_round_ddn.png"));
		model->MaterialAtIndex(0, 7)->AddTexture(RN::Texture::WithFile("models/sponza/vase_round_spec.png"));
		model->MaterialAtIndex(0, 7)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 7)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 7)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 7)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 9)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_arch_ddn.png"));
		model->MaterialAtIndex(0, 9)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_arch_spec.png"));
		model->MaterialAtIndex(0, 9)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 9)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 9)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 9)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 11)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_b_ddn.png"));
		model->MaterialAtIndex(0, 11)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_b_spec.png"));
		model->MaterialAtIndex(0, 11)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 11)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 11)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 11)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 15)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_a_ddn.png"));
		model->MaterialAtIndex(0, 15)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_a_spec.png"));
		model->MaterialAtIndex(0, 15)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 15)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 15)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 15)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 17)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_thorn_ddn.png"));
		model->MaterialAtIndex(0, 17)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_thorn_spec.png"));
		model->MaterialAtIndex(0, 17)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 17)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 17)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 17)->Define("RN_SPECMAP");
#endif
		
		RN::bullet::Shape *sponzaShape = new RN::bullet::TriangelMeshShape(model);
		RN::bullet::PhysicsMaterial *sponzaMaterial = new RN::bullet::PhysicsMaterial();
		
		sponzaShape->SetScale(RN::Vector3(0.2f));
		
		RN::bullet::RigidBody *sponza = new RN::bullet::RigidBody(sponzaShape, 0.0f);
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.2f));
		sponza->SetMaterial(sponzaMaterial);
		sponza->SetRotation(RN::Quaternion(RN::Vector3(0.0, 0.0, -90.0)));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
#if !TGWorldFeatureFreeCamera
		RN::Model *playerModel = RN::Model::WithFile("models/TiZeta/simplegirl.sgm");
		RN::Skeleton *playerSkeleton = RN::Skeleton::WithFile("models/TiZeta/simplegirl.sga");
		playerSkeleton->SetAnimation("cammina");
		
		_player = new Player(playerModel);
		_player->SetSkeleton(playerSkeleton);
		_player->SetPosition(RN::Vector3(1.0f, -1.0f, 0.0f));
		_player->SetScale(RN::Vector3(0.4f));
		_player->SetCamera(_camera);
		
		_camera->SetTarget(_player);
#endif
				
#if TGWorldFeatureParticles
		RN::Texture *texture = RN::Texture::WithFile("textures/particle.png");
		
		RN::ParticleMaterial *material = new RN::ParticleMaterial();
		material->AddTexture(texture);
		
		material->lifespan = 10.0f;
		material->minVelocity = RN::Vector3(0.0f, 0.5f, 0.0f);
		material->maxVelocity = RN::Vector3(0.0f, 0.15f, 0.0f);
		
		material->discard = false;
		material->depthwrite = false;
		material->blending = true;
		material->SetBlendMode(RN::Material::BlendMode::Interpolative);
		
		DustEmitter *emitter = new DustEmitter();
		emitter->SetMaterial(material);
		emitter->Cook(100.0f, 10);
		emitter->group = 1;
#endif
		
#if TGWorldFeatureLights
		_sunLight = new RN::Light(RN::Light::TypeDirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(0.0f, 0.0f, -90.0f)));
		_sunLight->_lightcam = _camera;
		_sunLight->ActivateSunShadows(true);
		_sunLight->SetColor(RN::Color(170, 170, 170));
		
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
		
		for(int i=0; i<500; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 70.0f - 35.0f, TGWorldRandom * 50.0f-10.0f, TGWorldRandom * 40.0f - 20.0f));
			light->SetRange((TGWorldRandom * 5.0f) + 2.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		}
#endif
		
		RN::Billboard *billboard = new RN::Billboard();
		
		billboard->SetTexture(RN::Texture::WithFile("textures/billboard.png"));
		billboard->SetScale(RN::Vector3(0.2f));
		billboard->SetRotation(RN::Quaternion(RN::Vector3(90.0f, 0.0f, 0.0f)));
		billboard->TranslateLocal(RN::Vector3(0.0f, 9.0f, 57.0f));
	}
	
	
	void World::CreateForest()
	{
		// Ground
		RN::Shader *terrainShader = new RN::Shader();
		terrainShader->SetShaderForType("shader/rn_Terrain.fsh", RN::Shader::ShaderType::FragmentShader);
		terrainShader->SetShaderForType("shader/rn_Texture1.vsh", RN::Shader::ShaderType::VertexShader);
		
		RN::Model *ground = RN::Model::WithFile("models/UberPixel/ground.sgm");
		ground->MaterialAtIndex(0, 0)->SetShader(terrainShader);
		
		RN::bullet::Shape *groundShape = new RN::bullet::TriangelMeshShape(ground);
		RN::bullet::PhysicsMaterial *groundMaterial = new RN::bullet::PhysicsMaterial();
		
		groundShape->SetScale(RN::Vector3(20.0f));
		
		RN::bullet::RigidBody *groundbody = new RN::bullet::RigidBody(groundShape, 0.0f);
		groundbody->SetModel(ground);
		groundbody->SetScale(RN::Vector3(20.0f));
		groundbody->SetMaterial(groundMaterial);
		
		
		RN::Entity *ent;
		
		RN::Model *building = RN::Model::WithFile("models/Sebastian/Old_Buildings.sgm");
		ent = new RN::Entity();
		ent->SetModel(building);
		ent->SetPosition(RN::Vector3(0.0f, 0.6f, 0.0f));
		
#if TGWorldFeatureNormalMapping && TGWorldFeatureLights		
		building->MaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/Sebastian/brick2-NM.png"));
		building->MaterialAtIndex(0, 0)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 0)->Define("RN_SPECULARITY");
		
		building->MaterialAtIndex(0, 1)->AddTexture(RN::Texture::WithFile("models/Sebastian/Concrete_A-NM.png"));
		building->MaterialAtIndex(0, 1)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 1)->Define("RN_SPECULARITY");
		
		building->MaterialAtIndex(0, 2)->AddTexture(RN::Texture::WithFile("models/Sebastian/Concrete_B-NM.png"));
		building->MaterialAtIndex(0, 2)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 2)->Define("RN_SPECULARITY");
		
		building->MaterialAtIndex(0, 3)->AddTexture(RN::Texture::WithFile("models/Sebastian/Concrete_C-NM.png"));
		building->MaterialAtIndex(0, 3)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 3)->Define("RN_SPECULARITY");
		
		building->MaterialAtIndex(0, 4)->AddTexture(RN::Texture::WithFile("models/Sebastian/props-NM.png"));
		building->MaterialAtIndex(0, 4)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 4)->Define("RN_SPECULARITY");
		
		building->MaterialAtIndex(0, 5)->AddTexture(RN::Texture::WithFile("models/Sebastian/Rooftiles_A-NM.png"));
		building->MaterialAtIndex(0, 5)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 5)->Define("RN_SPECULARITY");
#endif
		
		building = RN::Model::WithFile("models/Sebastian/Old_BuildingsDecals.sgm");
		building->MaterialAtIndex(0, 0)->culling = false;
		building->MaterialAtIndex(0, 0)->discard = true;
		building->MaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard;
		
		building->MaterialAtIndex(0, 1)->culling = false;
		building->MaterialAtIndex(0, 1)->discard = true;
		building->MaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard;
		
		ent = new RN::Entity();
		ent->SetModel(building);
		ent->SetPosition(RN::Vector3(0.0f, 0.6f, 0.0f));
		
#if TGWorldFeatureNormalMapping && TGWorldFeatureLights
		building->MaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/Sebastian/Decals-NM.png"));
		building->MaterialAtIndex(0, 0)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 0)->Define("RN_SPECULARITY");
		
		building->MaterialAtIndex(0, 1)->AddTexture(RN::Texture::WithFile("models/Sebastian/Decals-NM.png"));
		building->MaterialAtIndex(0, 1)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 1)->Define("RN_SPECULARITY");
#endif
		
		building = RN::Model::WithFile("models/Sebastian/Old_BuildingsPlants.sgm");
		building->MaterialAtIndex(0, 0)->culling = false;
		building->MaterialAtIndex(0, 0)->discard = true;
		building->MaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard;
		
		ent = new RN::Entity();
		ent->SetModel(building);
		ent->SetPosition(RN::Vector3(0.0f, 0.6f, 0.0f));
		
#if TGWorldFeatureNormalMapping && TGWorldFeatureLights
		building->MaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/Sebastian/plants-NM.png"));
		building->MaterialAtIndex(0, 0)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 0)->Define("RN_SPECULARITY");
#endif
		
		RN::Model *tree = RN::Model::WithFile("models/dexfuck/spruce2.sgm");
		tree->MaterialAtIndex(0, 0)->culling = false;
		tree->MaterialAtIndex(0, 0)->discard = true;
		tree->MaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		
		RN::InstancingNode *node;
		RN::Random::DualPhaseLCG dualPhaseLCG;
		dualPhaseLCG.Seed(0x1024);
		
		node = new RN::InstancingNode(tree);
		
		for(int i = 0; i < TGForestFeatureTrees; i += 1)
		{
			RN::Vector3 pos = RN::Vector3(dualPhaseLCG.RandomFloatRange(-100.0f, 100.0f), 0.0f, dualPhaseLCG.RandomFloatRange(-100.0f, 100.0f));
			if(pos.Length() < 10.0f)
				continue;
			
			ent = new RN::Entity();
			ent->SetModel(tree);
			ent->SetPosition(pos);
			ent->SetScale(RN::Vector3(dualPhaseLCG.RandomFloatRange(0.89f, 1.12f)));
			ent->SetRotation(RN::Vector3(dualPhaseLCG.RandomFloatRange(0.0f, 365.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
			
			if(i == 10)
			{
				ent->SetAction([](RN::SceneNode *node, float delta) {
					node->Translate(RN::Vector3(0.0f, 1.0f * delta, 0.0f));
				});
			}
		}
		
		RN::Model *grass = RN::Model::WithFile("models/dexfuck/grass01.sgm");
		grass->MaterialAtIndex(0, 0)->culling = false;
		grass->MaterialAtIndex(0, 0)->discard = true;
		grass->MaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		//grass->MaterialAtIndex(0, 0)->Define("PURPLE");
		
		node = new RN::InstancingNode(grass);
		
		for(int i=0; i<TGForestFeatureGras; i++)
		{
			RN::Vector3 pos = RN::Vector3(dualPhaseLCG.RandomFloatRange(-50.0f, 50.0f), 0.2f, dualPhaseLCG.RandomFloatRange(-50.0f, 50.0f));
			if(pos.Length() < 5.0f)
				continue;
			
			ent = new RN::Entity();
			ent->SetModel(grass);
			ent->SetPosition(pos);
			ent->SetScale(RN::Vector3(2.5f));
			ent->SetRotation(RN::Vector3(dualPhaseLCG.RandomFloatRange(0, 365.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
		}
		
#if !TGWorldFeatureFreeCamera
		RN::Model *playerModel = RN::Model::WithFile("models/TiZeta/simplegirl.sgm");
		RN::Skeleton *playerSkeleton = RN::Skeleton::WithFile("models/TiZeta/simplegirl.sga");
		playerSkeleton->SetAnimation("cammina");
		
		_player = new Player(playerModel);
		_player->SetSkeleton(playerSkeleton);
		_player->SetPosition(RN::Vector3(5.0f, 10.0f, 0.0f));
		_player->SetScale(RN::Vector3(0.4f));
		_player->SetCamera(_camera);
		
		_camera->SetTarget(_player);
#endif
		
#if TGWorldFeatureLights
		_sunLight = new RN::Light(RN::Light::TypeDirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(0.0f, 0.0f, -90.0f)));
		_sunLight->_lightcam = _camera;
		_sunLight->ActivateSunShadows(true);
		
		_spotLight = new RN::Light(RN::Light::TypeSpotLight);
		_spotLight->SetPosition(RN::Vector3(0.75f, -0.5f, 0.0f));
		_spotLight->SetRange(TGWorldSpotLightRange);
		_spotLight->SetAngle(0.9f);
		_spotLight->SetColor(RN::Color(0.5f, 0.5f, 0.5f));
		
#if TGWorldFeatureFreeCamera
		_camera->AttachChild(_spotLight);
#else
		_player->AttachChild(_spotLight);
#endif
		
/*		for(int i=0; i<200; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 140.0f - 70.0f, TGWorldRandom * 100.0f-20.0f, TGWorldRandom * 80.0f - 40.0f));
			light->SetRange((TGWorldRandom * 20.0f) + 10.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
			
			light->SetAction([](RN::Transform *transform, float delta) {
			 transform->Translate(RN::Vector3(0.5f * delta, 0.0f, 0.0));
			 });
		}*/
#endif
	}

}
