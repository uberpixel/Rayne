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
#define TGWorldFeatureWater			0

#define TGForestFeatureTrees 500
#define TGForestFeatureGras  0

#define TGWorldRandom (float)(rand())/RAND_MAX
#define TGWorldSpotLightRange 30.0f

namespace TG
{
	World::World() :
		RN::World("OctreeSceneManager")
	{
//		srand(time(0));
		
		_sunLight = 0;
		_finalcam = 0;
		_camera = 0;
		_spotLight = 0;
		_player = 0;
		_sponza = 0;
		
		_exposure = 1.0f;
		_whitepoint = 5.0f;
		
		_debugAttachment = new DebugDrawer();
		AddAttachment(_debugAttachment);
		
		CreateCameras();
		CreateSponza();
//		CreateForest();
//		CreateSibenik();
		
		RN::Input::GetSharedInstance()->Activate();
		RN::MessageCenter::GetSharedInstance()->AddObserver(kRNInputEventMessage, [&](RN::Message *message) {
			
			RN::Event *event = static_cast<RN::Event *>(message);
			if(event->GetType() == RN::Event::Type::KeyDown)
			{
				switch(event->GetCharacter())
				{
					case 'f':
						if(_spotLight)
							_spotLight->SetRange(_spotLight->GetRange() > 1.0f ? 0.0f : TGWorldSpotLightRange);
						break;
						
					case 'x':
						_debugAttachment->SetCamera(_debugAttachment->Camera() ? nullptr : _camera);
						break;
						
					default:
						break;
				}
			}
			
		}, this);
	}
	
	World::~World()
	{
		RN::Input::GetSharedInstance()->Deactivate();
		RN::MessageCenter::GetSharedInstance()->RemoveObserver(this);
		
		_camera->Release();
	}
	
	void World::Update(float delta)
	{
		RN::Input *input = RN::Input::GetSharedInstance();

#if TGWorldFeatureFreeCamera
		RN::Vector3 translation;
		RN::Vector3 rotation;
		
		const RN::Vector2& mouseDelta = input->GetMouseDelta();
		
		if(!(input->GetModifierKeys() & RN::KeyModifier::KeyControl))
		{
			rotation.x = mouseDelta.x;
			rotation.y = mouseDelta.y;
		}
		
		translation.x = (input->IsKeyPressed('d') - input->IsKeyPressed('a')) * 16.0f;
		translation.z = (input->IsKeyPressed('s') - input->IsKeyPressed('w')) * 16.0f;
		
		translation *= (input->GetModifierKeys() & RN::KeyModifier::KeyShift) ? 2.0f : 1.0f;
		
		_camera->Rotate(rotation);
		_camera->TranslateLocal(translation * delta);
#endif
		
		if(_sunLight != 0)
		{
			RN::Vector3 sunrot;
			sunrot.x = (input->IsKeyPressed('e') - input->IsKeyPressed('q')) * 20.0f * delta;
			sunrot.y = (input->IsKeyPressed('t') - input->IsKeyPressed('g')) * 10.0f * delta;
			_sunLight->Rotate(sunrot);
		}
		
		_exposure += (input->IsKeyPressed('u') - input->IsKeyPressed('j')) * delta*2.0f;
		_exposure = MIN(MAX(0.01f, _exposure), 10.0f);
		_whitepoint += (input->IsKeyPressed('i') - input->IsKeyPressed('k')) * delta;
		_whitepoint = MIN(MAX(0.01f, _whitepoint), 10.0f);
		RN::Renderer::GetSharedInstance()->SetHDRExposure(_exposure);
		RN::Renderer::GetSharedInstance()->SetHDRWhitePoint(_whitepoint);
	}
	
	void World::CreateCameras()
	{
		RN::Model *sky = RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png");
		sky->GetMaterialAtIndex(0, 0)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->GetMaterialAtIndex(0, 1)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->GetMaterialAtIndex(0, 2)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->GetMaterialAtIndex(0, 3)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->GetMaterialAtIndex(0, 4)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->GetMaterialAtIndex(0, 5)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		
#if TGWorldFeatureZPrePass
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatDepth);
		
		RN::Texture::Parameter depthparam;
		depthparam.format = RN::Texture::Format::Depth24I;
		depthparam.generateMipMaps = false;
		depthparam.maxMipMaps = 0;
		depthparam.wrapMode = RN::Texture::WrapMode::Clamp;
		
		_depthtex = new RN::Texture2D(depthparam);
		storage->SetDepthTarget(_depthtex);
		
		RN::Shader *depthShader = RN::ResourceCoordinator::GetSharedInstance()->GetResourceWithName<RN::Shader>(kRNResourceKeyDirectionalShadowDepthShader, nullptr);
		RN::Material *depthMaterial = new RN::Material(depthShader);
		
		_camera = new ThirdPersonCamera(storage, RN::Camera::FlagDefaults|RN::Camera::FlagNoLights);
		_camera->SetMaterial(depthMaterial);
		
		_finalcam = new RN::Camera(RN::Vector2(), RN::Texture::Format::RGBA32F, RN::Camera::FlagDefaults);
		_finalcam->SetClearMask(RN::Camera::ClearFlagColor);
		_finalcam->GetStorage()->SetDepthTarget(_depthtex);
		_finalcam->SetSkyCube(sky);
		_finalcam->renderGroup |= RN::Camera::RenderGroup1;
		_finalcam->SetPriority(5);
		
		_camera->AttachChild(_finalcam);
		_camera->SetPriority(10);
		_camera->Rotate(RN::Vector3(90.0f, 0.0f, 0.0f));
		
		_finalcam->SetDrawFramebufferShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
		
	#if TGWorldFeatureWater
		RN::Shader *updownShader = RN::Shader::WithFile("shader/rn_PPCopy");
		
		// Copy refraction to another texture
		RN::Material *copyFBOMaterial = new RN::Material(updownShader);
		copyFBOMaterial->Define("RN_COPYDEPTH");
		RN::Camera *copyRefract = new RN::Camera(_camera->GetFrame().Size(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		copyRefract->SetMaterial(copyFBOMaterial);
		_refractPipeline = _lightcam->AddPostProcessingPipeline("refractioncopy");
		_refractPipeline->AddStage(copyRefract, RN::RenderStage::Mode::ReUsePreviousStage);
	#endif
		
	#if TGWorldFeatureSSAO
		PPActivateSSAO(_finalcam);
	#endif
		
	#if TGWorldFeatureBloom
		PPActivateBloom(_finalcam);
	#endif
#else
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatComplete);
		storage->AddRenderTarget(RN::Texture::Format::RGBA32F);
		_camera = new ThirdPersonCamera(storage);
		_camera->SetSkyCube(sky);
		_camera->SetDrawFramebufferShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
		
	#if TGWorldFeatureSSAO
		PPActivateSSAO(_camera);
	#endif
		
	#if TGWorldFeatureBloom
		PPActivateBloom(_camera);
	#endif
#endif
	}
	
	void World::PPActivateSSAO(RN::Camera *cam)
	{
		RN::Shader *combineShader = RN::Shader::WithFile("shader/rn_PPCombine");
		RN::Shader *blurShader = RN::Shader::WithFile("shader/rn_BoxBlur");
		RN::Shader *updownShader = RN::Shader::WithFile("shader/rn_PPCopy");
		
		RN::Material *blurXMaterial = new RN::Material(blurShader);
		blurXMaterial->Define("RN_BLURX");
		
		RN::Material *blurYMaterial = new RN::Material(blurShader);
		blurYMaterial->Define("RN_BLURY");
		
		RN::Material *downMaterial = new RN::Material(updownShader);
		downMaterial->Define("RN_DOWNSAMPLE");
		
		// Surface normals
		RN::Shader *surfaceShader = RN::Shader::WithFile("shader/rn_SurfaceNormals");
		RN::Material *surfaceMaterial = new RN::Material(surfaceShader);
		
		RN::Camera *normalsCamera = new RN::Camera(RN::Vector2(), RN::Texture::Format::RGBA32F, RN::Camera::FlagInherit | RN::Camera::FlagNoSky | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatComplete);
		normalsCamera->SetMaterial(surfaceMaterial);
		//normalsCamera->GetStorage()->SetDepthTarget(_depthtex);
		//normalsCamera->SetClearMask(RN::Camera::ClearFlagColor);
		
		// SSAO stage
		RN::Texture *ssaoNoise = RN::Texture::WithFile("textures/rn_SSAONoise.png");
		
		RN::Shader *ssaoShader = RN::Shader::WithFile("shader/rn_SSAO");
		RN::Material *ssaoMaterial = new RN::Material(ssaoShader);
		ssaoMaterial->AddTexture(ssaoNoise);
		
		RN::Camera *ssaoCamera  = new RN::Camera(RN::Vector2(), RN::Texture::Format::R8, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoCamera->SetMaterial(ssaoMaterial);
		
		// Blur X
		RN::Camera *ssaoBlurX = new RN::Camera(RN::Vector2(), RN::Texture::Format::R8, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoBlurX->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *ssaoBlurY = new RN::Camera(RN::Vector2(), RN::Texture::Format::R8, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoBlurY->SetMaterial(blurYMaterial);
		
		// Combine stage
		RN::Material *ssaoCombineMaterial = new RN::Material(combineShader);
		ssaoCombineMaterial->AddTexture(ssaoBlurY->GetStorage()->GetRenderTarget());
		ssaoCombineMaterial->Define("MODE_GRAYSCALE");
		
		RN::Camera *ssaoCombineCamera  = new RN::Camera(RN::Vector2(), RN::Texture::Format::RGB888, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		ssaoCombineCamera->SetMaterial(ssaoCombineMaterial);
		
		// PP pipeline
		RN::PostProcessingPipeline *ssaoPipeline = cam->AddPostProcessingPipeline("SSAO");
		ssaoPipeline->AddStage(normalsCamera, RN::RenderStage::Mode::ReRender);
		ssaoPipeline->AddStage(ssaoCamera, RN::RenderStage::Mode::ReUsePreviousStage);
		ssaoPipeline->AddStage(ssaoBlurX, RN::RenderStage::Mode::ReUsePreviousStage);
		ssaoPipeline->AddStage(ssaoBlurY, RN::RenderStage::Mode::ReUsePreviousStage);
		ssaoPipeline->AddStage(ssaoCombineCamera, RN::RenderStage::Mode::ReUsePipeline);
	}
	
	void World::PPActivateBloom(RN::Camera *cam)
	{
		RN::Shader *combineShader = RN::Shader::WithFile("shader/rn_PPCombine");
		RN::Shader *blurShader = RN::Shader::WithFile("shader/rn_BoxBlur");
		RN::Shader *updownShader = RN::Shader::WithFile("shader/rn_PPCopy");
		
		RN::Material *blurXMaterial = new RN::Material(blurShader);
		blurXMaterial->Define("RN_BLURX");
		
		RN::Material *blurYMaterial = new RN::Material(blurShader);
		blurYMaterial->Define("RN_BLURY");
		
		RN::Material *downMaterial = new RN::Material(updownShader);
		downMaterial->Define("RN_DOWNSAMPLE");
		
		// Filter bright
		RN::Shader *filterBrightShader = RN::Shader::WithFile("shader/rn_FilterBright");
		RN::Material *filterBrightMaterial = new RN::Material(filterBrightShader);
		RN::Camera *filterBright = new RN::Camera(cam->GetFrame().Size() / 2.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		filterBright->SetMaterial(filterBrightMaterial);
		
		// Down sample
		RN::Camera *downSample4x = new RN::Camera(cam->GetFrame().Size() / 4.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample4x->SetMaterial(downMaterial);
		
		// Down sample
		RN::Camera *downSample8x = new RN::Camera(cam->GetFrame().Size() / 8.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample8x->SetMaterial(downMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXlow = new RN::Camera(cam->GetFrame().Size() / 8.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXlow->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYlow = new RN::Camera(cam->GetFrame().Size() / 8.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYlow->SetMaterial(blurYMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXhigh = new RN::Camera(cam->GetFrame().Size() / 4.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXhigh->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYhigh = new RN::Camera(cam->GetFrame().Size() / 4.0f, RN::Texture::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYhigh->SetMaterial(blurYMaterial);
		
		// Combine
		RN::Material *bloomCombineMaterial = new RN::Material(combineShader);
		bloomCombineMaterial->AddTexture(bloomBlurYhigh->GetStorage()->GetRenderTarget());
		
		RN::Camera *bloomCombine = new RN::Camera(RN::Vector2(0.0f), RN::Texture::Format::RGBA32F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomCombine->SetMaterial(bloomCombineMaterial);
		
		RN::PostProcessingPipeline *bloom = cam->AddPostProcessingPipeline("Bloom");
		bloom->AddStage(filterBright, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample4x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample8x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurXlow, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurYlow, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurXhigh, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurYhigh, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomCombine, RN::RenderStage::Mode::ReUsePipeline);
	}
	
	void World::CreateSponza()
	{
		_camera->ambient = RN::Vector4(0.127, 0.252, 0.393, 1.0f)*0.7f;
		
		// Sponza
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		model->GetMaterialAtIndex(0, 12)->discard = true;
		model->GetMaterialAtIndex(0, 12)->culling = false;
		model->GetMaterialAtIndex(0, 12)->override = RN::Material::OverrideGroupDiscard;
		
		model->GetMaterialAtIndex(0, 17)->discard = true;
		model->GetMaterialAtIndex(0, 17)->culling = false;
		model->GetMaterialAtIndex(0, 17)->override = RN::Material::OverrideGroupDiscard;
		
		model->GetMaterialAtIndex(0, 22)->discard = true;
		model->GetMaterialAtIndex(0, 22)->culling = false;
		model->GetMaterialAtIndex(0, 22)->override = RN::Material::OverrideGroupDiscard;
	
		
		RN::Entity *sponza = new RN::Entity();
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.2f));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
		_sponza = sponza;
		
//		TG::SmokeGrenade *smoke = new TG::SmokeGrenade();
//		smoke->Material()->AddTexture(_depthtex);
//		smoke->Material()->Define("RN_SOFTPARTICLE");
//		smoke->SetPosition(RN::Vector3(0.0f, -8.0f, 0.0f));
		
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
		
#if TGWorldFeatureLights
		_sunLight = new RN::Light(RN::Light::Type::DirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(0.0f, -90.0f, 0.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateDirectionalShadows(true, 2048);
		_sunLight->SetColor(RN::Color(170, 170, 170));
		
/*		_spotLight = new RN::Light(RN::Light::Type::SpotLight);
		_spotLight->SetPosition(RN::Vector3(0.75f, -0.5f, 0.0f));
		_spotLight->SetRange(TGWorldSpotLightRange);
		_spotLight->SetAngle(20.0f);
		_spotLight->SetColor(RN::Color(0.5f));
		_spotLight->ActivateSpotShadows();*/
		
#if TGWorldFeatureFreeCamera
//		_camera->AttachChild(_spotLight);
#else
		_player->AttachChild(_spotLight);
#endif
		
/*		RN::Light *light = new RN::Light();
		light->SetPosition(RN::Vector3(-21.0f, -5.0f, -5.0f));
		light->SetRange(15.0f);
		light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		light->ActivatePointShadows();
		
		light = new RN::Light();
		light->SetPosition(RN::Vector3(-21.0f, -5.0f, 5.0f));
		light->SetRange(15.0f);
		light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		light->ActivatePointShadows();
		
		light = new RN::Light();
		light->SetPosition(RN::Vector3(29.0f, -5.0f, -5.0f));
		light->SetRange(15.0f);
		light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		light->ActivatePointShadows();
		
		light = new RN::Light();
		light->SetPosition(RN::Vector3(29.0f, -5.0f, 5.0f));
		light->SetRange(15.0f);
		light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		light->ActivatePointShadows();
	*/
		for(int i=0; i<200; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 50.0f - 21.0f, TGWorldRandom * 20.0f-7.0f, TGWorldRandom * 21.0f - 10.0f));
			light->SetRange((TGWorldRandom * 3.0f) + 2.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
			float timeoffset = TGWorldRandom*10.0f;
			light->SetAction([timeoffset](RN::SceneNode *light, float delta) {
				RN::Vector3 pos = light->GetWorldPosition();
				float time = RN::Kernel::GetSharedInstance()->GetTime();
				time += timeoffset;
				pos.x += 0.05f*cos(time*2.0f);
				pos.z += 0.05f*sin(time*2.0f);
				light->SetWorldPosition(pos);
			});
//			light->ActivatePointShadows();
		}
#endif
		
		RN::Billboard *billboard = new RN::Billboard();
		
		billboard->SetTexture(RN::Texture::WithFile("textures/billboard.png"));
		billboard->SetScale(RN::Vector3(0.04f));
		billboard->GetMaterial()->blending = true;
		billboard->GetMaterial()->blendSource = GL_SRC_ALPHA;
		billboard->GetMaterial()->blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		billboard->GetMaterial()->depthwrite = false;
		billboard->GetMaterial()->depthtest = true;
		billboard->renderGroup = 1;
		billboard->SetRotation(RN::Quaternion(RN::Vector3(90.0f, 0.0f, 0.0f)));
		billboard->Translate(RN::Vector3(-17.35f, 12.0f, 0.7f));
		
		_camera->clipfar = 100.0f;
		_camera->UpdateProjection();
		
#if  TGWorldFeatureZPrePass
//		_lightcam->clipfar = 100.0f;
//		_lightcam->UpdateProjection();
		_finalcam->clipfar = 100.0f;
		_finalcam->UpdateProjection();
#endif
		
#if TGWorldFeatureWater
		RN::Water *water = new RN::Water((RN::Camera*)_finalcam, _refractPipeline->LastStage()->Camera()->Storage()->RenderTarget());
#endif
	}
	
	
	void World::CreateForest()
	{
		_camera->ambient = RN::Vector4(0.127, 0.252, 0.393, 1.0f)*2.0f;
		
		//ground
		RN::Model *ground = RN::Model::WithFile("models/UberPixel/ground.sgm");
		ground->GetMaterialAtIndex(0, 0)->Define("RN_TEXTURE_TILING", 5);
		RN::Entity *groundBody = new RN::Entity();
		groundBody->SetModel(ground);
		groundBody->SetScale(RN::Vector3(20.0f));
		
		//house
		RN::Model *house = RN::Model::WithFile("models/blendswap/cc0_timber_house/timber_house.sgm");
		RN::Entity *houseent = new RN::Entity();
		houseent->SetModel(house);
		houseent->SetWorldPosition(RN::Vector3(0.0f, 0.8f, 0.0f));
		

#define TREE_MODEL_COUNT 10
		RN::Model *trees[TREE_MODEL_COUNT];
		trees[0] = RN::Model::WithFile("models/pure3d/BirchTrees/birch2m.sgm");
		trees[0]->GetMaterialAtIndex(0, 0)->culling = false;
		trees[0]->GetMaterialAtIndex(0, 0)->discard = true;
		trees[0]->GetMaterialAtIndex(0, 0)->discardThreshold = 0.1f;
		trees[0]->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[0]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[0]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[0]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[0]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.1f;
		trees[0]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[0]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[0]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[0]->GetMaterialAtIndex(1, 0)->culling = false;
		trees[0]->GetMaterialAtIndex(1, 0)->discard = true;
		trees[0]->GetMaterialAtIndex(1, 0)->discardThreshold = 0.1f;
		trees[0]->GetMaterialAtIndex(1, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[0]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[0]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[0]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[0]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.1f;
		trees[0]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[0]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[0]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		trees[1] = RN::Model::WithFile("models/pure3d/BirchTrees/birch6m.sgm");
		trees[1]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[1]->GetMaterialAtIndex(0, 0)->culling = false;
		trees[1]->GetMaterialAtIndex(0, 0)->discard = true;
		trees[1]->GetMaterialAtIndex(0, 0)->discardThreshold = 0.1f;
		trees[1]->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[1]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[1]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[1]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[1]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[1]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.1f;
		trees[1]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[1]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		
		trees[2] = RN::Model::WithFile("models/pure3d/BirchTrees/birch11m.sgm");
		trees[2]->GetMaterialAtIndex(0, 0)->culling = false;
		trees[2]->GetMaterialAtIndex(0, 0)->discard = true;
		trees[2]->GetMaterialAtIndex(0, 0)->discardThreshold = 0.1f;
		trees[2]->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[2]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[2]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[2]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[2]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.1f;
		trees[2]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[2]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[2]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[2]->GetMaterialAtIndex(1, 0)->culling = false;
		trees[2]->GetMaterialAtIndex(1, 0)->discard = true;
		trees[2]->GetMaterialAtIndex(1, 0)->discardThreshold = 0.1f;
		trees[2]->GetMaterialAtIndex(1, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[2]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[2]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[2]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[2]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.1f;
		trees[2]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[2]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[2]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		trees[3] = RN::Model::WithFile("models/pure3d/BirchTrees/birch13m.sgm");
		trees[3]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[3]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[3]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[3]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.1f;
		trees[3]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[3]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[3]->GetMaterialAtIndex(0, 2)->culling = false;
		trees[3]->GetMaterialAtIndex(0, 2)->discard = true;
		trees[3]->GetMaterialAtIndex(0, 2)->discardThreshold = 0.1f;
		trees[3]->GetMaterialAtIndex(0, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[3]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[3]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[3]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[3]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[3]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.1f;
		trees[3]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[3]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[3]->GetMaterialAtIndex(1, 2)->culling = false;
		trees[3]->GetMaterialAtIndex(1, 2)->discard = true;
		trees[3]->GetMaterialAtIndex(1, 2)->discardThreshold = 0.1f;
		trees[3]->GetMaterialAtIndex(1, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[3]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		trees[4] = RN::Model::WithFile("models/pure3d/BirchTrees/birch18m.sgm");
		trees[4]->GetMaterialAtIndex(0, 0)->culling = false;
		trees[4]->GetMaterialAtIndex(0, 0)->discard = true;
		trees[4]->GetMaterialAtIndex(0, 0)->discardThreshold = 0.1f;
		trees[4]->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[4]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[4]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[4]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[4]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.1f;
		trees[4]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[4]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[4]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[4]->GetMaterialAtIndex(1, 0)->culling = false;
		trees[4]->GetMaterialAtIndex(1, 0)->discard = true;
		trees[4]->GetMaterialAtIndex(1, 0)->discardThreshold = 0.1f;
		trees[4]->GetMaterialAtIndex(1, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[4]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[4]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[4]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[4]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.1f;
		trees[4]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[4]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[4]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		trees[5] = RN::Model::WithFile("models/pure3d/BirchTrees/birch20m.sgm");
		trees[5]->GetMaterialAtIndex(0, 0)->culling = false;
		trees[5]->GetMaterialAtIndex(0, 0)->discard = true;
		trees[5]->GetMaterialAtIndex(0, 0)->discardThreshold = 0.1f;
		trees[5]->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[5]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[5]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[5]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[5]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[5]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[5]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.1f;
		trees[5]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[5]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		
		trees[6] = RN::Model::WithFile("models/pure3d/PineTrees/pine4m.sgm");
		trees[6]->GetMaterialAtIndex(0, 0)->culling = false;
		trees[6]->GetMaterialAtIndex(0, 0)->discard = true;
		trees[6]->GetMaterialAtIndex(0, 0)->discardThreshold = 0.5f;
		trees[6]->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[6]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[6]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[6]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.5f;
		trees[6]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[6]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(1, 0)->culling = false;
		trees[6]->GetMaterialAtIndex(1, 0)->discard = true;
		trees[6]->GetMaterialAtIndex(1, 0)->discardThreshold = 0.5f;
		trees[6]->GetMaterialAtIndex(1, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[6]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[6]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[6]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.5f;
		trees[6]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[6]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(2, 0)->culling = false;
		trees[6]->GetMaterialAtIndex(2, 0)->discard = true;
		trees[6]->GetMaterialAtIndex(2, 0)->discardThreshold = 0.5f;
		trees[6]->GetMaterialAtIndex(2, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[6]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		trees[6]->GetMaterialAtIndex(2, 1)->culling = false;
		trees[6]->GetMaterialAtIndex(2, 1)->discard = true;
		trees[6]->GetMaterialAtIndex(2, 1)->discardThreshold = 0.5f;
		trees[6]->GetMaterialAtIndex(2, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[6]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		
		trees[7] = RN::Model::WithFile("models/pure3d/PineTrees/pine7m.sgm");
		trees[7]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[7]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[7]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.5f;
		trees[7]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[7]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(0, 2)->culling = false;
		trees[7]->GetMaterialAtIndex(0, 2)->discard = true;
		trees[7]->GetMaterialAtIndex(0, 2)->discardThreshold = 0.5f;
		trees[7]->GetMaterialAtIndex(0, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[7]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[7]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[7]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.5f;
		trees[7]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[7]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(1, 2)->culling = false;
		trees[7]->GetMaterialAtIndex(1, 2)->discard = true;
		trees[7]->GetMaterialAtIndex(1, 2)->discardThreshold = 0.5f;
		trees[7]->GetMaterialAtIndex(1, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[7]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(2, 1)->culling = false;
		trees[7]->GetMaterialAtIndex(2, 1)->discard = true;
		trees[7]->GetMaterialAtIndex(2, 1)->discardThreshold = 0.5f;
		trees[7]->GetMaterialAtIndex(2, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[7]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		trees[7]->GetMaterialAtIndex(2, 2)->culling = false;
		trees[7]->GetMaterialAtIndex(2, 2)->discard = true;
		trees[7]->GetMaterialAtIndex(2, 2)->discardThreshold = 0.5f;
		trees[7]->GetMaterialAtIndex(2, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[7]->GetMaterialAtIndex(2, 2)->Define("RN_VEGETATION");
		
		trees[8] = RN::Model::WithFile("models/pure3d/PineTrees/pine9m.sgm");
		trees[8]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[8]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[8]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.5f;
		trees[8]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[8]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(0, 2)->culling = false;
		trees[8]->GetMaterialAtIndex(0, 2)->discard = true;
		trees[8]->GetMaterialAtIndex(0, 2)->discardThreshold = 0.5f;
		trees[8]->GetMaterialAtIndex(0, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[8]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[8]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[8]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.5f;
		trees[8]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[8]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(1, 2)->culling = false;
		trees[8]->GetMaterialAtIndex(1, 2)->discard = true;
		trees[8]->GetMaterialAtIndex(1, 2)->discardThreshold = 0.5f;
		trees[8]->GetMaterialAtIndex(1, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[8]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		trees[8]->GetMaterialAtIndex(2, 1)->culling = false;
		trees[8]->GetMaterialAtIndex(2, 1)->discard = true;
		trees[8]->GetMaterialAtIndex(2, 1)->discardThreshold = 0.5f;
		trees[8]->GetMaterialAtIndex(2, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[8]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		
		trees[9] = RN::Model::WithFile("models/pure3d/PineTrees/pine16m.sgm");
		trees[9]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(0, 1)->culling = false;
		trees[9]->GetMaterialAtIndex(0, 1)->discard = true;
		trees[9]->GetMaterialAtIndex(0, 1)->discardThreshold = 0.5f;
		trees[9]->GetMaterialAtIndex(0, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[9]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(0, 2)->culling = false;
		trees[9]->GetMaterialAtIndex(0, 2)->discard = true;
		trees[9]->GetMaterialAtIndex(0, 2)->discardThreshold = 0.5f;
		trees[9]->GetMaterialAtIndex(0, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[9]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(1, 1)->culling = false;
		trees[9]->GetMaterialAtIndex(1, 1)->discard = true;
		trees[9]->GetMaterialAtIndex(1, 1)->discardThreshold = 0.5f;
		trees[9]->GetMaterialAtIndex(1, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[9]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(1, 2)->culling = false;
		trees[9]->GetMaterialAtIndex(1, 2)->discard = true;
		trees[9]->GetMaterialAtIndex(1, 2)->discardThreshold = 0.5f;
		trees[9]->GetMaterialAtIndex(1, 2)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[9]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		trees[9]->GetMaterialAtIndex(2, 1)->culling = false;
		trees[9]->GetMaterialAtIndex(2, 1)->discard = true;
		trees[9]->GetMaterialAtIndex(2, 1)->discardThreshold = 0.5f;
		trees[9]->GetMaterialAtIndex(2, 1)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		trees[9]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		

		RN::Entity *ent;
		RN::InstancingNode *node;
		RN::Random::DualPhaseLCG dualPhaseLCG;
		dualPhaseLCG.Seed(0x1024);
		
		node = new RN::InstancingNode();
		node->SetModels(RN::Array::WithObjects(trees[0], trees[1], trees[2], trees[3], trees[4], trees[5], trees[6], trees[7], trees[8], trees[9], nullptr));
		
		for(int i = 0; i < TGForestFeatureTrees; i ++)
		{
			RN::Vector3 pos = RN::Vector3(dualPhaseLCG.RandomFloatRange(-100.0f, 100.0f), 0.0f, dualPhaseLCG.RandomFloatRange(-100.0f, 100.0f));
			
			if(pos.Length() < 10.0f)
				continue;
			
			ent = new RN::Entity();
			ent->SetFlags(ent->GetFlags() | RN::SceneNode::FlagStatic);
			ent->SetModel(trees[dualPhaseLCG.RandomInt32Range(0, TREE_MODEL_COUNT)]);
			ent->SetPosition(pos);
			ent->SetScale(RN::Vector3(dualPhaseLCG.RandomFloatRange(0.89f, 1.12f)));
			ent->SetRotation(RN::Vector3(dualPhaseLCG.RandomFloatRange(0.0f, 365.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
		}
		
		/*RN::Model *grass = RN::Model::WithFile("models/dexsoft/grass/grass_1.sgm");
		grass->GetMaterialAtIndex(0, 0)->culling = false;
		grass->GetMaterialAtIndex(0, 0)->discard = true;
		grass->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		
		node = new RN::InstancingNode(grass);
		node->SetPivot(_camera);
		node->SetLimit(200, 800);
		
		for(int i = 0; i < TGForestFeatureGras; i ++)
		{
			RN::Vector3 pos = RN::Vector3(dualPhaseLCG.RandomFloatRange(-50.0f, 50.0f), 0.2f, dualPhaseLCG.RandomFloatRange(-50.0f, 50.0f));
			if(pos.Length() < 5.0f)
				continue;
			
			ent = new RN::Entity();
			ent->SetFlags(ent->GetFlags() | RN::SceneNode::FlagStatic);
			ent->SetModel(grass);
			ent->SetPosition(pos);
			ent->SetScale(RN::Vector3(2.5f));
			ent->SetRotation(RN::Vector3(dualPhaseLCG.RandomFloatRange(0, 365.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
		}*/
		
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
		_sunLight = new RN::Light(RN::Light::Type::DirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(60.0f, -60.0f, 0.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateDirectionalShadows(true, 2048);
	
/*		for(int i=0; i<10; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 20.0f - 10.0f, TGWorldRandom * 5.0f, TGWorldRandom * 20.0f-10.0f));
			light->SetRange((TGWorldRandom * 20.0f) + 10.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		}*/
#endif
	}
	
	void World::CreateSibenik()
	{
		RN::Model *sibenik = RN::Model::WithFile("models/Dabrovic/sibenik/sibenik.sgm");
		RN::Entity *ent = new RN::Entity();
		ent->SetModel(sibenik);
		
		_sunLight = new RN::Light(RN::Light::Type::DirectionalLight);
		_sunLight->SetIntensity(5.0f);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(60.0f, -60.0f, 0.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateDirectionalShadows();
		
		_spotLight = new RN::Light(RN::Light::Type::SpotLight);
		_spotLight->SetPosition(RN::Vector3(0.75f, -0.5f, 0.0f));
		_spotLight->SetRange(TGWorldSpotLightRange);
		_spotLight->SetAngle(20.0f);
		_spotLight->SetColor(RN::Color(0.5f));
		_spotLight->ActivateSpotShadows(true, 8192);
		_camera->AttachChild(_spotLight);
		
		for(int i=0; i<0; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 20.0f - 10.0f, TGWorldRandom * 20.0f-15.0f, TGWorldRandom * 20.0f - 10.0f));
			light->SetRange((TGWorldRandom * 3.0f) + 2.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
			float timeoffset = TGWorldRandom*10.0f;
			light->SetAction([timeoffset](RN::SceneNode *light, float delta) {
				RN::Vector3 pos = light->GetWorldPosition();
				float time = RN::Kernel::GetSharedInstance()->GetTime();
				time += timeoffset;
				pos.x += 0.05f*cos(time*2.0f);
				pos.z += 0.05f*sin(time*2.0f);
				light->SetWorldPosition(pos);
			});
		}
	}
}
