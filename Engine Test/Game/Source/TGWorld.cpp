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

#define TGForestFeatureTrees 500
#define TGForestFeatureGras  10000

#define TGWorldRandom (float)(rand())/RAND_MAX
#define TGWorldSpotLightRange 30.0f

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
		_sponza = 0;
		
		_exposure = 1.0f;
		_whitepoint = 5.0f;
		
		_physicsAttachment = new RN::bullet::PhysicsWorld();
		_debugAttachment = new DebugDrawer();
		
		AddAttachment(_physicsAttachment->Autorelease());
		AddAttachment(_debugAttachment);
		
		CreateCameras();
//		CreateSponza();
		CreateForest();
//		CreateTest();
		
		RN::Input::SharedInstance()->Activate();
		RN::MessageCenter::SharedInstance()->AddObserver(kRNInputEventMessage, [&](RN::Message *message) {
			
			RN::Event *event = static_cast<RN::Event *>(message);
			if(event->EventType() == RN::Event::Type::KeyUp)
			{
				switch(event->Character())
				{
					case 'f':
						if(_spotLight)
							_spotLight->SetRange(_spotLight->Range() > 1.0f ? 0.0f : TGWorldSpotLightRange);
						break;
						
					case 'x':
						_debugAttachment->SetCamera(_debugAttachment->Camera() ? nullptr : _lightcam);
						break;
						
					default:
						break;
				}
			}
			
		}, this);
	}
	
	World::~World()
	{
		RN::Input::SharedInstance()->Deactivate();
		RN::MessageCenter::SharedInstance()->RemoveObserver(this);
		
		_camera->Release();
	}
	
	void World::Update(float delta)
	{
		RN::Input *input = RN::Input::SharedInstance();

#if TGWorldFeatureFreeCamera
		RN::Vector3 translation;
		RN::Vector3 rotation;
		
		const RN::Vector2& mouseDelta = input->MouseDelta();
		
		if(!(input->ModifierKeys() & RN::KeyModifier::KeyControl))
		{
			rotation.x = mouseDelta.x;
			rotation.z = mouseDelta.y;
		}
		
		translation.x = (input->KeyPressed('d') - input->KeyPressed('a')) * 16.0f;
		translation.z = (input->KeyPressed('s') - input->KeyPressed('w')) * 16.0f;
		
		translation *= (input->ModifierKeys() & RN::KeyModifier::KeyShift) ? 2.0f : 1.0f;
		
		_camera->Rotate(rotation);
		_camera->TranslateLocal(translation * delta);
#endif
		
		if(_sunLight != 0)
		{
			RN::Vector3 sunrot;
			sunrot.x = (input->KeyPressed('e') - input->KeyPressed('q')) * 5.0f;
			sunrot.z = (input->KeyPressed('t') - input->KeyPressed('g')) * 2.0f;
			_sunLight->Rotate(sunrot);
		}
		
		_exposure += (input->KeyPressed('u') - input->KeyPressed('j')) * delta*2.0f;
		_exposure = MIN(MAX(0.01f, _exposure), 10.0f);
		_whitepoint += (input->KeyPressed('i') - input->KeyPressed('k')) * delta;
		_whitepoint = MIN(MAX(0.01f, _whitepoint), 10.0f);
		RN::Renderer::SharedInstance()->SetHDRExposure(_exposure);
		RN::Renderer::SharedInstance()->SetHDRWhitePoint(_whitepoint);
	}
	
	void World::CreateCameras()
	{
#if TGWorldFeatureZPrePass
		RN::Shader *combineShader = RN::Shader::WithFile("shader/rn_PPCombine");
		RN::Shader *blurShader = RN::Shader::WithFile("shader/rn_BoxBlur");
		RN::Shader *updownShader = RN::Shader::WithFile("shader/rn_PPCopy");
		
		RN::Material *blurXMaterial = new RN::Material(blurShader);
		blurXMaterial->Define("RN_BLURX");
		
		RN::Material *blurYMaterial = new RN::Material(blurShader);
		blurYMaterial->Define("RN_BLURY");
		
		RN::Material *downMaterial = new RN::Material(updownShader);
		downMaterial->Define("RN_DOWNSAMPLE");
		//RN::Material *upMaterial = new RN::Material(updownShader);
		
		
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatDepth|RN::RenderStorage::BufferFormatStencil);
		
		RN::TextureParameter depthparam;
		depthparam.format = RN::TextureParameter::Format::DepthStencil;
		depthparam.generateMipMaps = false;
		depthparam.mipMaps = 0;
		depthparam.wrapMode = RN::TextureParameter::WrapMode::Clamp;
		
		_depthtex = new RN::Texture(depthparam);
		storage->SetDepthTarget(_depthtex);
		
		RN::Shader *depthShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightDepthShader);
		RN::Material *depthMaterial = new RN::Material(depthShader);
		
		_camera = new ThirdPersonCamera(storage);
		_camera->SetMaterial(depthMaterial);
		
		RN::Shader *downsampleShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightTileSampleShader);
		RN::Shader *downsampleFirstShader = RN::ResourcePool::SharedInstance()->ResourceWithName<RN::Shader>(kRNResourceKeyLightTileSampleFirstShader);

		RN::Model *sky = RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png");
		sky->MaterialAtIndex(0, 0)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->MaterialAtIndex(0, 1)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->MaterialAtIndex(0, 2)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->MaterialAtIndex(0, 3)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->MaterialAtIndex(0, 4)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		sky->MaterialAtIndex(0, 5)->ambient = RN::Color(10.0f, 10.0f, 10.0f, 1.0f);
		
		_lightcam = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagDefaults);
		_lightcam->SetClearMask(RN::Camera::ClearFlagColor);
		_lightcam->Storage()->SetDepthTarget(_depthtex);
		_lightcam->SetSkyCube(sky);
		_lightcam->renderGroup |= RN::Camera::RenderGroup1;//|RN::Camera::RenderGroup2;
		_lightcam->SetLightTiles(RN::Vector2(32.0f, 32.0f));
		
		RN::DownsamplePostProcessingPipeline *downsamplePipeline = new RN::DownsamplePostProcessingPipeline("downsample", _lightcam, _depthtex, downsampleFirstShader, downsampleShader, RN::TextureParameter::Format::RG32F);
		_camera->AttachPostProcessingPipeline(downsamplePipeline);
		_lightcam->ActivateTiledLightLists(downsamplePipeline->LastTarget());
		_lightcam->SetPriority(5);
		
		// Copy refraction to another texture
		RN::Material *copyFBOMaterial = new RN::Material(updownShader);
		copyFBOMaterial->Define("RN_COPYDEPTH");
		RN::Camera *copyRefract = new RN::Camera(_camera->Frame().Size(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		copyRefract->SetMaterial(copyFBOMaterial);
		_refractPipeline = _lightcam->AddPostProcessingPipeline("refractioncopy");
		_refractPipeline->AddStage(copyRefract, RN::RenderStage::Mode::ReUsePreviousStage);
		
		
		_finalcam = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagDefaults);
		_finalcam->SetClearMask(0);
		_finalcam->Storage()->SetDepthTarget(_depthtex);
		_finalcam->Storage()->SetRenderTarget(_lightcam->Storage()->RenderTarget());
		_finalcam->renderGroup = RN::Camera::RenderGroup2;
		_finalcam->SetPriority(0);
		
		_camera->AttachChild(_lightcam);
		_camera->AttachChild(_finalcam);
		_camera->SetPriority(10);
		_camera->Rotate(RN::Vector3(90.0f, 0.0f, 0.0f));
		
		
		_finalcam->SetDrawFramebufferShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
		
#if TGWorldFeatureSSAO
		// Surface normals
		RN::Shader *surfaceShader = RN::Shader::WithFile("shader/rn_SurfaceNormals");
		RN::Material *surfaceMaterial = new RN::Material(surfaceShader);
		
		RN::Camera *normalsCamera = new RN::Camera(RN::Vector2(), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagInherit | RN::Camera::FlagNoSky | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatComplete);
		normalsCamera->SetMaterial(surfaceMaterial);
		normalsCamera->Storage()->SetDepthTarget(_depthtex);
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
		RN::Camera *filterBright = new RN::Camera(_camera->Frame().Size() / 2.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		filterBright->SetMaterial(filterBrightMaterial);
		
		// Down sample
		RN::Camera *downSample4x = new RN::Camera(_camera->Frame().Size() / 4.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample4x->SetMaterial(downMaterial);
		
		// Down sample
		RN::Camera *downSample8x = new RN::Camera(_camera->Frame().Size() / 8.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample8x->SetMaterial(downMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXlow = new RN::Camera(_camera->Frame().Size() / 8.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXlow->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYlow = new RN::Camera(_camera->Frame().Size() / 8.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYlow->SetMaterial(blurYMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXhigh = new RN::Camera(_camera->Frame().Size() / 4.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXhigh->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYhigh = new RN::Camera(_camera->Frame().Size() / 4.0f, RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYhigh->SetMaterial(blurYMaterial);
	
		// Combine
		RN::Material *bloomCombineMaterial = new RN::Material(combineShader);
		bloomCombineMaterial->AddTexture(bloomBlurYhigh->Storage()->RenderTarget());
		
		RN::Camera *bloomCombine = new RN::Camera(RN::Vector2(0.0f), RN::TextureParameter::Format::RGBA32F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomCombine->SetMaterial(bloomCombineMaterial);
		
		RN::PostProcessingPipeline *bloom = _finalcam->AddPostProcessingPipeline("Bloom");
		bloom->AddStage(filterBright, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample4x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(downSample8x, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurXlow, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurYlow, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurXhigh, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomBlurYhigh, RN::RenderStage::Mode::ReUsePreviousStage);
		bloom->AddStage(bloomCombine, RN::RenderStage::Mode::ReUsePipeline);
#endif
		
#else
		RN::RenderStorage *storage = new RN::RenderStorage(RN::RenderStorage::BufferFormatComplete);
		storage->AddRenderTarget(RN::TextureParameter::Format::RGBA32F);
		_camera = new ThirdPersonCamera(storage);
#endif
	}
	
	void World::CreateSponza()
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
		model->MaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/sponza/lion_ddn.png", true));
		model->MaterialAtIndex(0, 0)->Define("RN_NORMALMAP");
		
		model->MaterialAtIndex(0, 1)->AddTexture(RN::Texture::WithFile("models/sponza/background_ddn.png", true));
		model->MaterialAtIndex(0, 1)->Define("RN_NORMALMAP");
		
		model->MaterialAtIndex(0, 2)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_c_ddn.png", true));
		model->MaterialAtIndex(0, 2)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_c_spec.png"));
		model->MaterialAtIndex(0, 2)->specular = RN::Color(1.0f, 1.0f, 1.0f, 5.0f);
		model->MaterialAtIndex(0, 2)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 2)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 2)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 3)->AddTexture(RN::Texture::WithFile("models/sponza/spnza_bricks_a_ddn.png", true));
		model->MaterialAtIndex(0, 3)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 3)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 3)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 4)->AddTexture(RN::Texture::WithFile("models/sponza/vase_ddn.png", true));
		model->MaterialAtIndex(0, 4)->Define("RN_NORMALMAP");
		
		model->MaterialAtIndex(0, 5)->AddTexture(RN::Texture::WithFile("models/sponza/chain_texture_ddn.png", true));
		model->MaterialAtIndex(0, 5)->specular = RN::Color(0.5f, 0.5f, 0.5f, 1.0f);
		model->MaterialAtIndex(0, 5)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 5)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 6)->AddTexture(RN::Texture::WithFile("models/sponza/vase_plant_spec.png"));
		model->MaterialAtIndex(0, 6)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 6)->Define("RN_SPECMAP");
		model->MaterialAtIndex(0, 6)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 7)->AddTexture(RN::Texture::WithFile("models/sponza/vase_round_ddn.png", true));
		model->MaterialAtIndex(0, 7)->AddTexture(RN::Texture::WithFile("models/sponza/vase_round_spec.png"));
		model->MaterialAtIndex(0, 7)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 7)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 7)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 7)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 9)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_arch_ddn.png", true));
		model->MaterialAtIndex(0, 9)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_arch_spec.png"));
		model->MaterialAtIndex(0, 9)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 9)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 9)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 9)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 11)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_b_ddn.png", true));
		model->MaterialAtIndex(0, 11)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_b_spec.png"));
		model->MaterialAtIndex(0, 11)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 11)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 11)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 11)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 15)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_a_ddn.png", true));
		model->MaterialAtIndex(0, 15)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_column_a_spec.png"));
		model->MaterialAtIndex(0, 15)->specular = RN::Color(1.0f, 1.0f, 1.0f, 1.0f);
		model->MaterialAtIndex(0, 15)->Define("RN_NORMALMAP");
		model->MaterialAtIndex(0, 15)->Define("RN_SPECULARITY");
		model->MaterialAtIndex(0, 15)->Define("RN_SPECMAP");
		
		model->MaterialAtIndex(0, 16)->specular = RN::Color(0.02f, 0.02f, 0.02f, 32.0f);
		model->MaterialAtIndex(0, 16)->Define("RN_SPECULARITY");
		
		model->MaterialAtIndex(0, 17)->AddTexture(RN::Texture::WithFile("models/sponza/sponza_thorn_ddn.png", true));
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
		
		_sponza = sponza;
		
		TG::SmokeGrenade *smoke = new TG::SmokeGrenade();
		smoke->Material()->AddTexture(_depthtex);
		smoke->Material()->Define("RN_SOFTPARTICLE");
		smoke->SetPosition(RN::Vector3(0.0f, -8.0f, 0.0f));
		
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
		_sunLight = new RN::Light(RN::Light::TypeDirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(0.0f, 0.0f, -90.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateSunShadows(true, 1024.0f);
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
		for(int i=0; i<300; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 70.0f - 35.0f, TGWorldRandom * 30.0f-10.0f, TGWorldRandom * 40.0f - 20.0f));
			light->SetRange((TGWorldRandom * 5.0f) + 2.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		}
#endif
		
		RN::Billboard *billboard = new RN::Billboard();
		
		billboard->SetTexture(RN::Texture::WithFile("textures/billboard.png"));
		billboard->SetScale(RN::Vector3(0.04f));
		billboard->Material()->blending = true;
		billboard->Material()->blendSource = GL_SRC_ALPHA;
		billboard->Material()->blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		billboard->Material()->depthwrite = false;
		billboard->Material()->depthtest = true;
		billboard->renderGroup = 1;
		billboard->SetRotation(RN::Quaternion(RN::Vector3(90.0f, 0.0f, 0.0f)));
		billboard->Translate(RN::Vector3(-14.4f, 8.5f, 0.1f));
		
		_camera->clipfar = 100.0f;
		_camera->UpdateProjection();
		_lightcam->clipfar = 100.0f;
		_lightcam->UpdateProjection();
		_finalcam->clipfar = 100.0f;
		_finalcam->UpdateProjection();
		
//		RN::Water *water = new RN::Water((RN::Camera*)_finalcam, _refractPipeline->LastStage()->Camera()->Storage()->RenderTarget());
	}
	
	
	void World::CreateForest()
	{
		// Ground
		RN::Model *ground = RN::Model::WithFile("models/UberPixel/ground.sgm");
		ground->MaterialAtIndex(0, 0)->Define("RN_TEXTURE_TILING", 8);
		
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
		building->MaterialAtIndex(0, 0)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
		
		building->MaterialAtIndex(0, 1)->AddTexture(RN::Texture::WithFile("models/Sebastian/Concrete_A-NM.png"));
		building->MaterialAtIndex(0, 1)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 1)->Define("RN_SPECULARITY");
		building->MaterialAtIndex(0, 1)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
		
		building->MaterialAtIndex(0, 2)->AddTexture(RN::Texture::WithFile("models/Sebastian/Concrete_B-NM.png"));
		building->MaterialAtIndex(0, 2)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 2)->Define("RN_SPECULARITY");
		building->MaterialAtIndex(0, 2)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
		
		building->MaterialAtIndex(0, 3)->AddTexture(RN::Texture::WithFile("models/Sebastian/Concrete_C-NM.png"));
		building->MaterialAtIndex(0, 3)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 3)->Define("RN_SPECULARITY");
		building->MaterialAtIndex(0, 3)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
		
		building->MaterialAtIndex(0, 4)->AddTexture(RN::Texture::WithFile("models/Sebastian/props-NM.png"));
		building->MaterialAtIndex(0, 4)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 4)->Define("RN_SPECULARITY");
		building->MaterialAtIndex(0, 4)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
		
		building->MaterialAtIndex(0, 5)->AddTexture(RN::Texture::WithFile("models/Sebastian/Rooftiles_A-NM.png"));
		building->MaterialAtIndex(0, 5)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 5)->Define("RN_SPECULARITY");
		building->MaterialAtIndex(0, 5)->specular = RN::Color(0.1f, 0.1f, 0.1f, 20.0f);
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
		building->MaterialAtIndex(0, 0)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
		
		building->MaterialAtIndex(0, 1)->AddTexture(RN::Texture::WithFile("models/Sebastian/Decals-NM.png"));
		building->MaterialAtIndex(0, 1)->Define("RN_NORMALMAP");
		building->MaterialAtIndex(0, 1)->Define("RN_SPECULARITY");
		building->MaterialAtIndex(0, 1)->specular = RN::Color(0.02f, 0.02f, 0.02f, 10.0f);
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
		building->MaterialAtIndex(0, 0)->specular = RN::Color(0.2f, 0.2f, 0.2f, 30.0f);
#endif
		
		RN::Model *tree = RN::Model::WithFile("models/dexfuck/spruce2.sgm");
		tree->MaterialAtIndex(0, 0)->culling = false;
		tree->MaterialAtIndex(0, 0)->discard = true;
		tree->MaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		tree->MaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		
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
		_sunLight->SetLightCamera(_camera);
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
	
	void World::CreateTest()
	{
		// Ground
		RN::Model *ground = RN::Model::WithFile("models/UberPixel/ground.sgm");
		ground->MaterialAtIndex(0, 0)->Define("RN_TEXTURE_TILING", 8);
		
		RN::Entity *ent = new RN::Entity();
		ent->SetModel(ground);
		
		
		_sunLight = new RN::Light(RN::Light::TypeDirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(0.0f, 0.0f, -90.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateSunShadows(true);
	}

}
