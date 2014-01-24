//
//  TGWorld.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGWorld.h"
#include "TGSun.h"

#define TGWorldFeatureLights        1
#define TGWorldFeatureNormalMapping 1
#define TGWorldFeatureFreeCamera    1
#define TGWorldFeatureZPrePass		0
#define TGWorldFeatureBloom			0
#define TGWorldFeatureSSAO          0
#define TGWorldFeatureWater			0

#define TGForestFeatureTrees 500
#define TGForestFeatureGras  300000

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
		
		_frameCapturing = false;
		
		_exposure = 1.0f;
		_whitepoint = 5.0f;
		
		_cutScene = new CutScene();
		
		_debugAttachment = new DebugDrawer();
		AddAttachment(_debugAttachment);
	}
	
	World::~World()
	{
		RN::Input::GetSharedInstance()->Deactivate();
		RN::MessageCenter::GetSharedInstance()->RemoveObserver(this);
		
		_camera->Release();
	}
	
	void World::LoadOnThread(RN::Thread *thread)
	{
		CreateCameras();
		//CreateSponza();
		//CreateGrass();
		CreateForest();
		//CreateSibenik();
		
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
						
					case 'c':
						ToggleFrameCapturing();
						break;
						
					case 'k':
					{
						_cutScene->Reset();
						
						{
							BasicAnimation<RN::Vector3> *animation = new BasicAnimation<RN::Vector3>();
							animation->SetFromValue(RN::Vector3(-22.962570, 10.0f, -10.640179));
							animation->SetToValue(RN::Vector3(20.381281, 10.0f, -10.640179));
							animation->SetDuration(15.0f);
							animation->SetApplierFunction(std::bind(&RN::Camera::SetWorldPosition, _camera, std::placeholders::_1));
							
							_cutScene->AddAnimation(animation);
							animation->Release();
						}
						
						// Around first plier
						{
							BasicAnimation<RN::Vector3> *posAnimation = new BasicAnimation<RN::Vector3>();
							posAnimation->SetBeginTime(15.0f);
							posAnimation->SetFromValue(RN::Vector3(20.381281, 10.0f, -10.640179));
							posAnimation->SetToValue(RN::Vector3(24.716537, 10.0f, -3.987202));
							posAnimation->SetDuration(5.0f);
							posAnimation->SetApplierFunction(std::bind(&RN::Camera::SetWorldPosition, _camera, std::placeholders::_1));
							
							BasicAnimation<RN::Vector3> *rotAnimation = new BasicAnimation<RN::Vector3>();
							rotAnimation->SetBeginTime(15.0f);
							rotAnimation->SetFromValue(RN::Vector3(180.000000, -35.0f, 0.000000));
							rotAnimation->SetToValue(RN::Vector3(90.000000, -35.0f, 0.000000));
							rotAnimation->SetDuration(5.0f);
							rotAnimation->SetApplierFunction(std::bind(&RN::Camera::SetWorldRotation, _camera, std::placeholders::_1));
							
							_cutScene->AddAnimation(posAnimation);
							_cutScene->AddAnimation(rotAnimation);
							
							posAnimation->Release();
							rotAnimation->Release();
						}
						
						{
							BasicAnimation<RN::Vector3> *posAnimation = new BasicAnimation<RN::Vector3>();
							posAnimation->SetBeginTime(20.0f);
							posAnimation->SetFromValue(RN::Vector3(24.716537, 10.0f, -3.987202));
							posAnimation->SetToValue(RN::Vector3(24.716537, 10.0f, 5.089204));
							posAnimation->SetDuration(10.0f);
							posAnimation->SetApplierFunction(std::bind(&RN::Camera::SetWorldPosition, _camera, std::placeholders::_1));
							
							_cutScene->AddAnimation(posAnimation);
							
							posAnimation->Release();
						}
						
						// Around second plier
						{
							BasicAnimation<RN::Vector3> *posAnimation = new BasicAnimation<RN::Vector3>();
							posAnimation->SetBeginTime(30.0f);
							posAnimation->SetFromValue(RN::Vector3(24.716537, 10.0f, 5.089204));
							posAnimation->SetToValue(RN::Vector3(20.182526, 7.785197, 10.740821));
							posAnimation->SetDuration(5.0f);
							posAnimation->SetApplierFunction(std::bind(&RN::Camera::SetWorldPosition, _camera, std::placeholders::_1));
							
							BasicAnimation<RN::Vector3> *rotAnimation = new BasicAnimation<RN::Vector3>();
							rotAnimation->SetBeginTime(30.0f);
							rotAnimation->SetFromValue(RN::Vector3(90.000000, -35.0f, 0.000000));
							rotAnimation->SetToValue(RN::Vector3(0.0f, -5.0000, 0.000000));
							rotAnimation->SetDuration(5.0f);
							rotAnimation->SetApplierFunction(std::bind(&RN::Camera::SetWorldRotation, _camera, std::placeholders::_1));
							
							_cutScene->AddAnimation(posAnimation);
							_cutScene->AddAnimation(rotAnimation);
							
							posAnimation->Release();
							rotAnimation->Release();
						}
						
						{
							BasicAnimation<RN::Vector3> *posAnimation = new BasicAnimation<RN::Vector3>();
							posAnimation->SetBeginTime(35.0f);
							posAnimation->SetFromValue(RN::Vector3(20.182526, 7.785197, 10.740821));
							posAnimation->SetToValue(RN::Vector3(-17.017281, 7.785197, 10.618670));
							posAnimation->SetDuration(15.0f);
							posAnimation->SetApplierFunction(std::bind(&RN::Camera::SetWorldPosition, _camera, std::placeholders::_1));
							
							_cutScene->AddAnimation(posAnimation);
							
							posAnimation->Release();
						}
						
						_camera->SetRotation(RN::Quaternion(RN::Vector3(-180.000000, -35.0f, 0.000000)));
						
						break;
					}
						
					case 'p':
					{
						RN::Vector3 position = _camera->GetWorldPosition();
						RN::Vector3 euler = _camera->GetWorldEulerAngle();
						
						RNDebug("{%f, %f, %f}", position.x, position.y, position.z);
						RNDebug("{%f, %f, %f}", euler.x, euler.y, euler.z);
						
						break;
					}
						
					default:
						break;
				}
			}
			
		}, this);
	}
	
	bool World::SupportsBackgroundLoading() const
	{
		return true;
	}
	
	
	void World::ToggleFrameCapturing()
	{
		if(!_frameCapturing)
		{
			_frameCapturing = true;
			
			RN::Kernel::GetSharedInstance()->SetFixedDelta(1.0f / 30.0f);
			RecordFrame();
		}
		else
		{
			_frameCapturing = false;
			RN::Kernel::GetSharedInstance()->SetFixedDelta(0.0f);
		}
	}
	
	void World::RecordFrame()
	{
		if(!_frameCapturing)
			return;
		
		RN::Renderer::GetSharedInstance()->RequestFrameCapture([](RN::FrameCapture *capture) {
			
			RN::Data *data = capture->GetData(RN::FrameCapture::Format::RGBA8888);
			std::stringstream file;
			
			std::string path = RN::FileManager::GetSharedInstance()->GetNormalizedPathFromFullpath("~/Desktop");
			
			file << path << "/Capture/Capture_" << capture->GetFrame() << ".bmp";
			std::string base = RN::PathManager::Basepath(file.str());
			
			if(!RN::PathManager::PathExists(base))
				RN::PathManager::CreatePath(base);
			
			
			uint8 fileHeader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
			uint8 infoHeader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
			
			size_t w = capture->GetWidth();
			size_t h = capture->GetHeight();
			
			size_t padding   = (4 - w % 4) % 4;
			size_t dataSize  = w * h * 3 + (h * padding);
			size_t totalSize = dataSize + 14 + 40;
			
			fileHeader[2] = (totalSize) & 0xff;
			fileHeader[3] = (totalSize >> 8) & 0xff;
			fileHeader[4] = (totalSize >> 16) & 0xff;
			fileHeader[5] = (totalSize >> 24) & 0xff;
			
			infoHeader[ 4] = (w) & 0xff;
			infoHeader[ 5] = (w >> 8) & 0xff;
			infoHeader[ 6] = (w >> 16) & 0xff;
			infoHeader[ 7] = (w >> 24) & 0xff;
			infoHeader[ 8] = (h) & 0xff;
			infoHeader[ 9] = (h >> 8) & 0xff;
			infoHeader[10] = (h >> 16) & 0xff;
			infoHeader[11] = (h >> 24) & 0xff;
			
			RN::Data *bmpdata = new RN::Data();
			bmpdata->Append(fileHeader, 14);
			bmpdata->Append(infoHeader, 40);
			
			uint8 *pixelData = reinterpret_cast<uint8 *>(data->GetBytes());
			uint8 *row = new uint8[capture->GetWidth() * 3];
			uint8 pad[3] = { 0 };
			
			for(size_t i = 0; i < capture->GetHeight(); i ++)
			{
				uint32 *pixel = reinterpret_cast<uint32 *>(pixelData + (w * 4) * ((h - i) - 1));
				uint8  *temp  = row;
				
				for(size_t j = 0; j < capture->GetWidth(); j ++)
				{
					*temp ++ = ((*pixel) >> 16) & 0xff;
					*temp ++ = ((*pixel) >> 8) & 0xff;
					*temp ++ = ((*pixel) >> 0) & 0xff;
					
					pixel ++;
				}
				
				bmpdata->Append(row, capture->GetWidth() * 3);
				bmpdata->Append(pad, padding);
			}
			
			bmpdata->WriteToFile(file.str());
			bmpdata->Release();
		});
	}
	
	void World::Update(float delta)
	{
		RecordFrame();
		
		if(_cutScene->HasAnimations())
			_cutScene->Update(delta);
		
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
		_exposure = std::min(std::max(0.01f, _exposure), 10.0f);
		_whitepoint += (input->IsKeyPressed('i') - input->IsKeyPressed('k')) * delta;
		_whitepoint = std::min(std::max(0.01f, _whitepoint), 10.0f);
		RN::Renderer::GetSharedInstance()->SetHDRExposure(_exposure);
		RN::Renderer::GetSharedInstance()->SetHDRWhitePoint(_whitepoint);
		
		
		//_sunLight->SetColor(RN::Color(1.0f, 0.0f, 0.0f));
		//_camera->fogcolor = RN::Color(1.0f, 0.0f, 0.0f);
		
		RN::Color color = _sunLight->GetAmbientColor();
		RN::Vector4 ambient(0.127, 0.252, 0.393, 1.0);
		
		_camera->ambient = RN::Vector4(color.r, color.g, color.b, 1.0) * (ambient * 2.0f);
		_camera->fogcolor = _sunLight->GetFogColor();
	}
	
	void World::CreateCameras()
	{
		RN::Model *sky = RN::Model::WithSkyCube("textures/sky_up.png", "textures/sky_down.png", "textures/sky_left.png", "textures/sky_right.png", "textures/sky_front.png", "textures/sky_back.png", "shader/rn_Sky2");
		for(int i = 0; i < 6; i++)
		{
			sky->GetMaterialAtIndex(0, i)->ambient = RN::Color(6.0f, 6.0f, 6.0f, 1.0f);
			sky->GetMaterialAtIndex(0, i)->Define("RN_ATMOSPHERE");
		}
		
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
		
		_finalcam->SetBlitShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
		
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
		storage->AddRenderTarget(RN::Texture::Format::RGB16F);
		_camera = new ThirdPersonCamera(storage);
		_camera->renderGroup = RN::Camera::RenderGroup0|RN::Camera::RenderGroup1;
		_camera->SetSkyCube(sky);
		_camera->SetBlitShader(RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap"));
		
	#if TGWorldFeatureSSAO
		PPActivateSSAO(_camera);
	#endif
		
	#if TGWorldFeatureBloom
		PPActivateBloom(_camera);
	#endif
		
		PPActivateFXAA(_camera);
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
		
		RN::Camera *normalsCamera = new RN::Camera(RN::Vector2(), RN::Texture::Format::RGB16F, RN::Camera::FlagInherit | RN::Camera::FlagNoSky | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatComplete);
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
		
		RN::Camera *ssaoCombineCamera  = new RN::Camera(RN::Vector2(), RN::Texture::Format::RGB16F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
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
		RN::Camera *filterBright = new RN::Camera(cam->GetFrame().Size() / 2.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		filterBright->SetMaterial(filterBrightMaterial);
		
		// Down sample
		RN::Camera *downSample4x = new RN::Camera(cam->GetFrame().Size() / 4.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample4x->SetMaterial(downMaterial);
		
		// Down sample
		RN::Camera *downSample8x = new RN::Camera(cam->GetFrame().Size() / 8.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		downSample8x->SetMaterial(downMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXlow = new RN::Camera(cam->GetFrame().Size() / 8.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXlow->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYlow = new RN::Camera(cam->GetFrame().Size() / 8.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYlow->SetMaterial(blurYMaterial);
		
		// Blur X
		RN::Camera *bloomBlurXhigh = new RN::Camera(cam->GetFrame().Size() / 4.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurXhigh->SetMaterial(blurXMaterial);
		
		// Blur Y
		RN::Camera *bloomBlurYhigh = new RN::Camera(cam->GetFrame().Size() / 4.0f, RN::Texture::Format::RGB16F, RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		bloomBlurYhigh->SetMaterial(blurYMaterial);
		
		// Combine
		RN::Material *bloomCombineMaterial = new RN::Material(combineShader);
		bloomCombineMaterial->AddTexture(bloomBlurYhigh->GetStorage()->GetRenderTarget());
		
		RN::Camera *bloomCombine = new RN::Camera(RN::Vector2(0.0f), RN::Texture::Format::RGB16F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
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
	
	void World::PPActivateFXAA(RN::Camera *cam)
	{
		RN::Shader *tonemappingShader = RN::Shader::WithFile("shader/rn_DrawFramebufferTonemap");
		RN::Shader *fxaaShader = RN::Shader::WithFile("shader/rn_FXAA");
		
		// FXAA
		RN::Material *tonemappingMaterial = new RN::Material(tonemappingShader);
		RN::Material *fxaaMaterial = new RN::Material(fxaaShader);
		
		RN::Camera *tonemappingCam = new RN::Camera(RN::Vector2(0.0f), RN::Texture::Format::RGB16F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		tonemappingCam->SetMaterial(tonemappingMaterial);
		
		RN::Camera *fxaaCam = new RN::Camera(RN::Vector2(0.0f), RN::Texture::Format::RGB16F, RN::Camera::FlagInherit | RN::Camera::FlagUpdateStorageFrame, RN::RenderStorage::BufferFormatColor);
		fxaaCam->SetMaterial(fxaaMaterial);
		
		RN::PostProcessingPipeline *fxaa = cam->AddPostProcessingPipeline("FXAA");
		fxaa->AddStage(tonemappingCam, RN::RenderStage::Mode::ReUsePipeline);
		fxaa->AddStage(fxaaCam, RN::RenderStage::Mode::ReUsePipeline);
	}
	
	void World::CreateSponza()
	{
		_camera->ambient = RN::Vector4(0.127, 0.252, 0.393, 1.0f)*0.7f;
		_camera->SetPosition(RN::Vector3(15.0, 0.0, 0.0));
		
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
/*		_sunLight = new RN::Light(RN::Light::Type::DirectionalLight);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(0.0f, -90.0f, 0.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateDirectionalShadows(true, 2048);
		_sunLight->SetColor(RN::Color(170, 170, 170));*/
		
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
		}
		_sunLight = new Sun();
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateShadows();
#endif
		
		RN::Billboard *billboard = new RN::Billboard();
		
		billboard->SetTexture(RN::Texture::WithFile("textures/billboard.png"));
		billboard->SetScale(RN::Vector3(0.04f));
		billboard->GetMaterial()->blending = true;
		billboard->GetMaterial()->blendSource = GL_SRC_ALPHA;
		billboard->GetMaterial()->blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		billboard->GetMaterial()->depthwrite = false;
		billboard->GetMaterial()->depthtest = true;
		billboard->SetRenderGroup(1);
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
	
	bool World::PositionBlocked(RN::Vector3 position, RN::Entity **obstacles, int count)
	{
		for(int n = 0; n < count; n++)
		{
			if(obstacles[n]->GetBoundingBox().Contains(position))
				return true;
		}
		return false;
	}
	
	
	void World::CreateForest()
	{
		RN::Progress *progress = RN::Progress::GetActiveProgress();
		progress->SetTotalUnits(100);
		
		_camera->ambient = RN::Vector4(0.127, 0.252, 0.393, 1.0f)*2.0f;
		_camera->SetPosition(RN::Vector3(0.0f, 2.0f, 0.0f));
		
		//ground
		RN::Model *ground = RN::Model::WithFile("models/UberPixel/ground.sgm");
		ground->GetMaterialAtIndex(0, 0)->Define("RN_TEXTURE_TILING", 5);
		ground->GetMaterialAtIndex(0, 0)->culling = false;
		ground->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideCulling;
		
		RN::Entity *groundBody = new RN::Entity();
		groundBody->SetModel(ground);
		groundBody->SetScale(RN::Vector3(20.0f));
	
		//house
/*		RN::Model *house = RN::Model::WithFile("models/blendswap/cc0_timber_house/timber_house.sgm");
		RN::Entity *houseent = new RN::Entity();
		houseent->SetModel(house);
		houseent->SetWorldPosition(RN::Vector3(0.0f, 0.8f, 0.0f));*/
		
		//tavern
		RN::Model *tavern = RN::Model::WithFile("models/dexsoft/tavern/tavern_main.sgm");
		RN::Entity *tavernent = new RN::Entity();
		tavernent->SetModel(tavern);
		tavernent->SetWorldPosition(RN::Vector3(0.0f, 0.0f, 0.0f));
		progress->IncrementCompletedUnits(5);
		
		RN::Model *house2 = RN::Model::WithFile("models/dexsoft/medieval_1/f1_house02.sgm");
		RN::Entity *house2ent = new RN::Entity();
		house2ent->SetModel(house2);
		house2ent->SetWorldPosition(RN::Vector3(0.0f, 0.0f, 30.0f));
		progress->IncrementCompletedUnits(5);
		
		RN::Model *ruin4 = RN::Model::WithFile("models/dexsoft/ruins/ruins_house4.sgm");
		RN::Entity *ruin4ent = new RN::Entity();
		ruin4ent->SetModel(ruin4);
		ruin4ent->SetWorldPosition(RN::Vector3(-30.0f, 0.0f, 0.0f));
		progress->IncrementCompletedUnits(5);
		
		/*RN::Model *dwarf = RN::Model::WithFile("models/psionic/dwarf/dwarf1.x");
		RN::Entity *dwarfent = new RN::Entity();
		dwarfent->SetModel(dwarf);
		dwarfent->GetSkeleton()->CopyAnimation("AnimationSet0", "idle", 292, 325);
		dwarfent->GetSkeleton()->SetAnimation("idle");
		dwarfent->SetAction([](RN::SceneNode *node, float delta) {
			RN::Entity *dwarfent = static_cast<RN::Entity*>(node);
			dwarfent->GetSkeleton()->Update(delta*15.0f);
		});
		dwarfent->SetWorldPosition(RN::Vector3(0.0f, 0.1f, 20.0f));
		dwarfent->SetScale(RN::Vector3(0.3f));
		
		RN::Model *rat = RN::Model::WithFile("models/psionic/rat/rat.b3d");
		RN::Entity *ratent = new RN::Entity();
		ratent->SetModel(rat);
		ratent->GetSkeleton()->CopyAnimation("", "run", 1, 10);
		ratent->GetSkeleton()->SetAnimation("run");
		ratent->SetAction([](RN::SceneNode *node, float delta) {
			RN::Entity *ratent = static_cast<RN::Entity*>(node);
			ratent->GetSkeleton()->Update(delta*20.0f);
			ratent->SetPosition(ratent->GetPosition()+RN::Vector3(-delta*2.0f, 0.0f, 0.0f));
			if(ratent->GetPosition().x < -15)
				ratent->SetPosition(RN::Vector3(10.0f, 0.2f, 0.0f));
		});
		ratent->SetScale(RN::Vector3(0.05f));
		ratent->SetRotation(RN::Vector3(90.0f, 0.0f, 0.0f));
		ratent->SetPosition(RN::Vector3(0.0f, 0.2f, 0.0f));
		
		RN::Model *ninja = RN::Model::WithFile("models/psionic/ninja/ninja.b3d");
		RN::Entity *ninjaent = new RN::Entity();
		ninjaent->SetModel(ninja);
		ninjaent->GetSkeleton()->CopyAnimation("", "idle", 206, 250);
		ninjaent->GetSkeleton()->SetAnimation("idle");
		ninjaent->SetAction([](RN::SceneNode *node, float delta) {
			RN::Entity *ninjaent = static_cast<RN::Entity*>(node);
			ninjaent->GetSkeleton()->Update(delta*15.0f);
		});
		ninjaent->SetWorldPosition(RN::Vector3(-17.0f, 0.1f, 17.0f));
		ninjaent->SetScale(RN::Vector3(0.27f));
		
		RN::Model *druid = RN::Model::WithFile("models/arteria3d/FemaleDruid/Female Druid.x");
		RN::Entity *druident = new RN::Entity();
		druident->SetModel(druid);
		druident->GetSkeleton()->SetAnimation("WizardCombatReadyA");
		druident->SetAction([](RN::SceneNode *node, float delta) {
			RN::Entity *druident = static_cast<RN::Entity*>(node);
			druident->GetSkeleton()->Update(delta*50.0f);
		});
		druident->SetWorldPosition(RN::Vector3(0.0f, 0.25f, 0.0f));
		druident->SetScale(RN::Vector3(0.01f));*/
		
		
		RN::Entity *obstacles[3];
		obstacles[0] = tavernent;
		obstacles[1] = house2ent;
		obstacles[2] = ruin4ent;
		

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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		
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
		progress->IncrementCompletedUnits(5);
		

		RN::Entity *ent;
		RN::InstancingNode *node;
		RN::Random::DualPhaseLCG dualPhaseLCG;
		dualPhaseLCG.Seed(0x1024);
		
		node = new RN::InstancingNode();
		node->SetModels(RN::Array::WithObjects(trees[0], trees[1], trees[2], trees[3], trees[4], trees[5], trees[6], trees[7], trees[8], trees[9], nullptr));
		node->SetPivot(_camera);
		
		for(int i = 0; i < TGForestFeatureTrees; i ++)
		{
			RN::Vector3 pos = RN::Vector3(dualPhaseLCG.RandomFloatRange(-200.0f, 200.0f), 0.0f, dualPhaseLCG.RandomFloatRange(-200.0f, 200.0f));
			
			if(PositionBlocked(pos+RN::Vector3(0.0f, 0.5f, 0.0f), obstacles, 3))
				continue;
			
			ent = new RN::Entity();
			ent->SetFlags(ent->GetFlags() | RN::SceneNode::FlagStatic);
			ent->SetModel(trees[dualPhaseLCG.RandomInt32Range(0, TREE_MODEL_COUNT)]);
			ent->SetPosition(pos);
			ent->SetScale(RN::Vector3(dualPhaseLCG.RandomFloatRange(0.89f, 1.12f)));
			ent->SetRotation(RN::Vector3(dualPhaseLCG.RandomFloatRange(0.0f, 360.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
		}
		
		PlaceEntitiesOnGround(node, groundBody);
		
		RN::Model *grass = RN::Model::WithFile("models/dexsoft/grass/grass_1.sgm");
		grass->GetMaterialAtIndex(0, 0)->culling = false;
		grass->GetMaterialAtIndex(0, 0)->discard = true;
		grass->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard|RN::Material::OverrideCulling;
		grass->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		grass->GetMaterialAtIndex(0, 0)->Define("RN_GRASS");
		
		node = new RN::InstancingNode(grass);
		node->SetRenderGroup(1);
		node->SetPivot(_camera);
		node->SetMode(RN::InstancingNode::Mode::Thinning | RN::InstancingNode::Mode::Clipping);
		node->SetCellSize(32.0f);
		node->SetClippingRange(16.0f);
		node->SetThinningRange(128.0f);
		
		for(int i = 0; i < TGForestFeatureGras; i ++)
		{
			RN::Vector3 pos = RN::Vector3(dualPhaseLCG.RandomFloatRange(-200.0f, 200.0f), 0.2f, dualPhaseLCG.RandomFloatRange(-200.0f, 200.0f));
			
			if(PositionBlocked(pos+RN::Vector3(0.0f, 1.0f, 0.0f), obstacles, 3))
				continue;
			
			ent = new RN::Entity();
			ent->SetFlags(ent->GetFlags() | RN::SceneNode::FlagStatic);
			ent->SetModel(grass);
			ent->SetPosition(pos);
			ent->SetScale(RN::Vector3(dualPhaseLCG.RandomFloatRange(1.5f, 2.0f)));
			ent->SetRotation(RN::Vector3(dualPhaseLCG.RandomFloatRange(0, 360.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
		}
		
		PlaceEntitiesOnGround(node, groundBody);
		
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
		_sunLight = new Sun();
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateShadows();
	
/*		for(int i=0; i<10; i++)
		{
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(TGWorldRandom * 20.0f - 10.0f, TGWorldRandom * 5.0f, TGWorldRandom * 20.0f-10.0f));
			light->SetRange((TGWorldRandom * 20.0f) + 10.0f);
			light->SetColor(RN::Color(TGWorldRandom, TGWorldRandom, TGWorldRandom));
		}*/
#endif
		
		progress->ResignActive();
	}
	
	void World::PlaceEntitiesOnGround(RN::SceneNode *node, RN::SceneNode *ground)
	{
		RN::ThreadPool::Batch *batch = RN::ThreadPool::GetSharedInstance()->CreateBatch();
		const RN::Array *children = node->GetChildren();
		
		RN::Progress *progress = RN::Progress::GetActiveProgress()->IntermediateProgressAcountingFor(15);
		progress->SetTotalUnits(children->GetCount());
		
		children->Enumerate<RN::Entity>([&](RN::Entity *entity, size_t index, bool *stop) {
			
			batch->AddTask([&, entity] {
				RN::Vector3 pos = entity->GetPosition();
				
				pos.y += 100.0f;
				pos.y = GetGroundHeight(pos, ground);
				
				entity->SetPosition(pos);
				progress->IncrementCompletedUnits(1);
				
			});
		});
		
		batch->Commit();
		batch->Wait();
		
		progress->ResignActive();
	}
	
	float World::GetGroundHeight(const RN::Vector3 &position, RN::SceneNode *ground)
	{
		float length = -100.0f;
		RN::Hit hit = ground->CastRay(position, RN::Vector3(0.0f, length, 0.0f));
		
		if(hit.node == ground)
			return position.y + length * hit.distance;
		
		return position.y;
	}
	
	void World::CreateGrass()
	{
		RN::Model *grass = RN::Model::WithFile("models/dexsoft/grass/grass_1.sgm");
		grass->GetMaterialAtIndex(0, 0)->culling = false;
		grass->GetMaterialAtIndex(0, 0)->discard = true;
		grass->GetMaterialAtIndex(0, 0)->override = RN::Material::OverrideGroupDiscard | RN::Material::OverrideCulling;
		grass->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		grass->GetMaterialAtIndex(0, 0)->Define("RN_GRASS");
		
		RN::InstancingNode *node = new RN::InstancingNode(grass);
		RN::Random::MersenneTwister random;
		
		for(int i = 0; i < 100000; i ++)
		{
			RN::Vector3 pos = RN::Vector3(random.RandomFloatRange(-100.0f, 100.0f), 0.0f, random.RandomFloatRange(-100.0f, 100.0f));

			RN::Entity *ent = new RN::Entity();
			ent->SetFlags(ent->GetFlags() | RN::SceneNode::FlagStatic);
			ent->SetModel(grass);
			ent->SetPosition(pos);
			ent->SetScale(random.RandomFloatRange(2.5f, 3.0f));
			ent->SetRotation(RN::Vector3(random.RandomFloatRange(0, 360.0f), 0.0f, 0.0f));
			
			node->AttachChild(ent);
		}
		
		
		_camera->ambient = RN::Vector4(0.127, 0.252, 0.393, 1.0f) * 2.0f;
		
		_sunLight = new Sun();
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(60.0f, -60.0f, 0.0f)));
	}
	
	void World::CreateSibenik()
	{
		RN::Model *sibenik = RN::Model::WithFile("models/Dabrovic/sibenik/sibenik.sgm");
		RN::Entity *ent = new RN::Entity();
		ent->SetModel(sibenik);
		
		_sunLight = new Sun();
		_sunLight->SetIntensity(5.0f);
		_sunLight->SetRotation(RN::Quaternion(RN::Vector3(60.0f, -60.0f, 0.0f)));
		_sunLight->SetLightCamera(_camera);
		_sunLight->ActivateShadows();
		
		_spotLight = new RN::Light(RN::Light::Type::SpotLight);
		_spotLight->SetPosition(RN::Vector3(0.75f, -0.5f, 0.0f));
		_spotLight->SetRange(TGWorldSpotLightRange);
		_spotLight->SetAngle(20.0f);
		_spotLight->SetColor(RN::Color(0.5f));
		_spotLight->ActivateShadows();
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
