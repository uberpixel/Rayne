//
//  __TMP__CameraManager.cpp
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#include "__TMP__CameraManager.h"
#include "__TMP__World.h"

namespace __TMP__
{
	CameraManager::CameraManager() : _defaultPreviewWindowResolution(960, 540), _vrWindow(nullptr), _vrCamera(nullptr), _headCamera(nullptr), _previewCamera(nullptr), _previewWindow(nullptr), _copyEyeToScreenMaterial(nullptr), _vrDebugWindow(nullptr), _cameraTargetAmbientColor(RN::Color::Black()), _cameraTargetAmbientColorChangeRate(RN::Color::Black()), _cameraTargetAmbientColorCompletedCallback(nullptr), _cameraTargetAmbientColorIsWaitingForLastFrame(false), _resetPositionAndRotation(false)
	{
		RN::Dictionary *resolutionDictionary = RN::Settings::GetSharedInstance()->GetEntryForKey<RN::Dictionary>(RNCSTR("RNResolution"));
		if(resolutionDictionary)
		{
			RN::Number *widthNumber = resolutionDictionary->GetObjectForKey<RN::Number>(RNCSTR("width"));
			RN::Number *heightNumber = resolutionDictionary->GetObjectForKey<RN::Number>(RNCSTR("height"));

			if(widthNumber)
			{
				_defaultPreviewWindowResolution.x = widthNumber->GetFloatValue();
			}

			if(heightNumber)
			{
				_defaultPreviewWindowResolution.y = heightNumber->GetFloatValue();
			}
		}

#if RN_PLATFORM_MAC_OS
		_wantsPreviewWindow = false;
		_wantsPreviewCamera = false;
		_wantsVRDebugWindow = false;
		_msaa = 0;
#elif RN_PLATFORM_ANDROID
		_wantsPreviewWindow = false;
		_wantsPreviewCamera = false;
		_wantsVRDebugWindow = false;
		_msaa = 4;
#else
		_wantsPreviewWindow = true;
		_wantsPreviewCamera = false;
		_wantsVRDebugWindow = false;
		_msaa = 8;
#endif
	}

	CameraManager::~CameraManager()
	{
		ClearPipeline();
	}

	void CameraManager::Setup(RN::VRWindow *vrWindow)
	{
		_vrWindow = vrWindow;

#if RN_PLATFORM_ANDROID
		_wantsVRDebugWindow == (_vrWindow == nullptr);
#endif
		
		if(!_wantsPreviewWindow && !_vrWindow) _wantsPreviewWindow = true;

		GeneratePipeline();
	}

	RN::VRHMDTrackingState::Mode CameraManager::Update(float delta)
	{
		UpdateForWindowSize();

		if(_vrCamera)
		{
			_vrCamera->UpdateVRWindow(delta);
		}

		ResetPositionAndRotation();
		MovePancakeCamera(delta);
		UpdateCameraAmbientColor(delta);
		//UpdatePreviewCamera(delta);
		
		if(_vrCamera)
		{
			return _vrCamera->GetHMDTrackingState().mode;
		}
		else
		{
			return RN::VRHMDTrackingState::Mode::Rendering;
		}
	}

	void CameraManager::UpdateForWindowSize() const
	{
		if(!_vrWindow || !_previewWindow || !_copyEyeToScreenMaterial) return;

		RN::Vector2 eyeResolution((_vrWindow->GetSize().x - _vrWindow->GetEyePadding()) / 2.0f, _vrWindow->GetSize().y);
		RN::Vector2 windowResolution(_previewWindow->GetSize());
		float eyeAspect = eyeResolution.x / eyeResolution.y;
		float windowAspect = windowResolution.x / windowResolution.y;
		RN::Vector4 sourceRectangle = 0.0f;

		if(eyeAspect < windowAspect)
		{
			sourceRectangle.z = eyeResolution.x;
			sourceRectangle.w = eyeResolution.x / windowAspect;
			sourceRectangle.x = 0.0f;
			sourceRectangle.y = (eyeResolution.y - sourceRectangle.w) * 0.5f;
		}
		else
		{
			sourceRectangle.z = eyeResolution.y * windowAspect;
			sourceRectangle.w = eyeResolution.y;
			sourceRectangle.x = (eyeResolution.x - sourceRectangle.z) * 0.5f;
			sourceRectangle.y = 0.0f;
		}

		_copyEyeToScreenMaterial->SetDiffuseColor(RN::Color(sourceRectangle.x / _vrWindow->GetSize().x,
															sourceRectangle.y / _vrWindow->GetSize().y, (sourceRectangle.z / _vrWindow->GetSize().x),
															(sourceRectangle.w / _vrWindow->GetSize().y)));
	}

	void CameraManager::SetPreviewWindowEnabled(bool enable)
	{
		if(enable == _wantsPreviewWindow) return;

		_wantsPreviewWindow = enable;
		RegeneratePipeline();
	}

	void CameraManager::SetPreviewCameraEnabled(bool enable)
	{
		if(enable == _wantsPreviewCamera) return;

		_wantsPreviewCamera = enable;
		RegeneratePipeline();
	}

	void CameraManager::ResetPositionAndRotationDelayed()
	{
		//This needs to happen delayed, because game objects are removed at the end of the frame and may otherwise override the camera position again
		_resetPositionAndRotation = true;
	}

	void CameraManager::ResetPositionAndRotation()
	{
		if(!_resetPositionAndRotation) return;
		_resetPositionAndRotation = false;
		
		if(_vrCamera)
		{
			_vrCamera->SetWorldPosition(RN::Vector3(0.0f, -1.8f, 0.0f));
			_vrCamera->SetWorldRotation(RN::Quaternion());
		}
		else if(_headCamera)
		{
			_headCamera->SetWorldPosition(RN::Vector3());
			_headCamera->SetWorldRotation(RN::Quaternion());
		}

		if(_previewCamera)
		{
			_previewCamera->SetWorldPosition(_headCamera->GetWorldPosition());
			_previewCamera->SetWorldRotation(_headCamera->GetWorldRotation());
		}
	}

	RN::SceneNode *CameraManager::GetHeadSceneNode() const
	{
		if(_vrCamera)
		{
			return _vrCamera->GetHead();
		}
		else
		{
			return _headCamera;
		}
	}

	void CameraManager::SetCameraAmbientColor(const RN::Color &targetColor, float changeRate, std::function<void(void)> completed)
	{
		_cameraTargetAmbientColor = targetColor;
		RN::Color fromColor;
		if(_vrCamera)
		{
			fromColor = _vrCamera->GetEye(0)->GetAmbientColor();
		}
		else if(_previewCamera)
		{
			fromColor = _previewCamera->GetAmbientColor();
		}
		_cameraTargetAmbientColorChangeRate = (targetColor - fromColor) * changeRate;
		_cameraTargetAmbientColorCompletedCallback = completed;
		_cameraTargetAmbientColorIsWaitingForLastFrame = false;
	}

	void CameraManager::UpdateCameraAmbientColor(float delta)
	{
		if(_cameraTargetAmbientColorIsWaitingForLastFrame)
		{
			_cameraTargetAmbientColorIsWaitingForLastFrame = false;
			if(_cameraTargetAmbientColorCompletedCallback) _cameraTargetAmbientColorCompletedCallback();
		}

		if(_cameraTargetAmbientColorChangeRate == RN::Color::Black()) return;
		if(delta > 0.2f) return;

		RN::Color colorChange = _cameraTargetAmbientColorChangeRate * delta;

		RN::Color fromColor;
		if(_vrCamera)
		{
			fromColor = _vrCamera->GetEye(0)->GetAmbientColor();
		}
		else if(_previewCamera)
		{
			fromColor = _previewCamera->GetAmbientColor();
		}

		RN::Color colorDiff = _cameraTargetAmbientColor - fromColor;
		if(colorDiff.r * colorDiff.r + colorDiff.g * colorDiff.g + colorDiff.b * colorDiff.b <= colorChange.r * colorChange.r + colorChange.g * colorChange.g + colorChange.b * colorChange.b)
		{
			colorChange = colorDiff;
			_cameraTargetAmbientColorChangeRate = RN::Color::Black();
			_cameraTargetAmbientColorIsWaitingForLastFrame = true;
		}

		RN::Color newColor = fromColor + colorChange;

		if(_vrCamera)
		{
			_vrCamera->GetEye(0)->SetAmbientColor(newColor);
			_vrCamera->GetEye(1)->SetAmbientColor(newColor);
		}

		if(_previewCamera)
		{
			_previewCamera->SetAmbientColor(newColor);
		}
	}

	void CameraManager::MovePancakeCamera(float delta)
	{
		if(!_vrWindow && _headCamera && _headCamera->GetChildren()->GetCount() == 0)
		{
			RN::InputManager *manager = RN::InputManager::GetSharedInstance();

			RN::Vector3 rotation(0.0);

			rotation.x = manager->GetMouseDelta().x;
			rotation.y = manager->GetMouseDelta().y;
			rotation = -rotation;

			RN::Vector3 translation(0.0);

			translation.x = ((int)manager->IsControlToggling(RNCSTR("D")) - (int)manager->IsControlToggling(RNCSTR("A"))) * 15.0f;
			translation.z = ((int)manager->IsControlToggling(RNCSTR("S")) - (int)manager->IsControlToggling(RNCSTR("W"))) * 15.0f;
			if(manager->IsControlToggling(RNCSTR("SHIFT"))) translation *= 4.0f;

			_headCamera->Rotate(rotation * delta * 15.0f);
			_headCamera->TranslateLocal(translation * delta);
		}
	}

	void CameraManager::UpdatePreviewCamera(float delta)
	{
		if(!_previewCamera || !_vrCamera) return;

		_previewCamera->SetWorldPosition(_headCamera->GetWorldPosition());

		RN::Quaternion rotationDiffQuaternion = _headCamera->GetWorldRotation() * _previewCamera->GetWorldRotation().GetConjugated();
		rotationDiffQuaternion.Normalize();
		RN::Vector4 rotationDiffAxisAngle = rotationDiffQuaternion.GetAxisAngle();
		rotationDiffAxisAngle.w = (rotationDiffAxisAngle.w > 0.0f? 1.0f : -1.0f) * std::min(std::abs(rotationDiffAxisAngle.w), std::abs(rotationDiffAxisAngle.w) * delta * 10.0f);

		_previewCamera->SetWorldRotation(RN::Quaternion(rotationDiffAxisAngle) * _previewCamera->GetWorldRotation());
	}

	void CameraManager::ClearPipeline()
	{
		World *world = World::GetSharedInstance();

		if(_copyEyeToScreenMaterial)
		{
			_copyEyeToScreenMaterial = nullptr;
		}

		if(_headCamera)
		{
			world->RemoveNode(_headCamera);
			_headCamera = nullptr;
		}

		if(_vrCamera)
		{
			world->RemoveNode(_vrCamera);
			_vrCamera = nullptr;
		}

		if(_previewCamera)
		{
			world->RemoveNode(_previewCamera);
			_previewCamera = nullptr;
		}

		SafeRelease(_previewWindow);
		SafeRelease(_vrDebugWindow);
	}

	void CameraManager::GeneratePipeline()
	{
		World *world = World::GetSharedInstance();

		if(_wantsPreviewWindow || !_vrWindow)
		{
			RN::Window::SwapChainDescriptor swapchainDescriptor(RN::Texture::Format::RGBA_16F, (_msaa > 1) ? RN::Texture::Format::Invalid : RN::Texture::Format::Depth_32F);
			swapchainDescriptor.vsync = false;
			_previewWindow = RN::Renderer::GetActiveRenderer()->CreateAWindow(_defaultPreviewWindowResolution, RN::Screen::GetMainScreen(), swapchainDescriptor);
			_previewWindow->Show();
		}

		if(_vrWindow)
		{
			RN::PostProcessingStage *monitorPass = nullptr;
			if(_wantsPreviewWindow && !_wantsPreviewCamera)
			{
				RN::ShaderLibrary *shaderLibrary = World::GetSharedInstance()->GetShaderLibrary();
				monitorPass = new RN::PostProcessingStage();
				_copyEyeToScreenMaterial = RN::Material::WithShaders(shaderLibrary->GetShaderWithName(RNCSTR("pp_vertex")), shaderLibrary->GetShaderWithName(RNCSTR("pp_blit_fragment")));
				monitorPass->SetFramebuffer(_previewWindow->GetFramebuffer());
				monitorPass->SetMaterial(_copyEyeToScreenMaterial);
				monitorPass->Autorelease();
			}

			if(_wantsVRDebugWindow)
			{
				_vrDebugWindow = RN::Renderer::GetActiveRenderer()->CreateAWindow(RN::Vector2(1920, 1080), RN::Screen::GetMainScreen());
			}

			_vrCamera = new RN::VRCamera(_vrWindow, monitorPass, _msaa, _vrDebugWindow);

			_vrCamera->SetClipFar(10000.0f);
			_vrCamera->GetEye(0)->SetAmbientColor(_cameraTargetAmbientColor);
			_vrCamera->GetEye(1)->SetAmbientColor(_cameraTargetAmbientColor);
			_vrCamera->GetEye(0)->GetRenderPass()->SetClearColor(RN::Color::Black());
			_vrCamera->GetEye(1)->GetRenderPass()->SetClearColor(RN::Color::Black());

			_headCamera = new RN::Camera();
			_headCamera->SetFlags(_headCamera->GetFlags() | RN::Camera::Flags::NoRender);
			_vrCamera->GetHead()->AddChild(_headCamera->Autorelease());
			world->AddNode(_vrCamera->Autorelease());
		}

		if(_wantsPreviewCamera || !_vrWindow)
		{
			_previewCamera = new RN::Camera();
			_previewCamera->SetFOV(60.0f);
			_previewCamera->SetClipFar(10000.0f);

			_previewCamera->SetAmbientColor(_cameraTargetAmbientColor);
			_previewCamera->GetRenderPass()->SetClearColor(RN::Color::Black());
			_previewCamera->GetRenderPass()->SetFlags(RN::RenderPass::Flags::ClearColor | RN::RenderPass::Flags::ClearDepthStencil);

			if(_msaa > 1)
			{
				RN::Texture *msaaTexture = RN::Texture::WithDescriptor(RN::Texture::Descriptor::With2DRenderTargetFormatAndMSAA(RN::Texture::Format::RGBA_16F, _previewWindow->GetSize().x, _previewWindow->GetSize().y, _msaa));
				RN::Texture *msaaDepthTexture = RN::Texture::WithDescriptor(RN::Texture::Descriptor::With2DRenderTargetFormatAndMSAA(RN::Texture::Format::Depth_32F, _previewWindow->GetSize().x, _previewWindow->GetSize().y, _msaa));
				RN::Framebuffer *msaaFramebuffer = RN::Renderer::GetActiveRenderer()->CreateFramebuffer(_previewWindow->GetSize());
				msaaFramebuffer->SetColorTarget(RN::Framebuffer::TargetView::WithTexture(msaaTexture));
				msaaFramebuffer->SetDepthStencilTarget(RN::Framebuffer::TargetView::WithTexture(msaaDepthTexture));

				_previewCamera->GetRenderPass()->SetFramebuffer(msaaFramebuffer);
				RN::PostProcessingAPIStage *resolvePass = new RN::PostProcessingAPIStage(RN::PostProcessingAPIStage::Type::ResolveMSAA);
				resolvePass->SetFramebuffer(_previewWindow->GetFramebuffer());
				_previewCamera->GetRenderPass()->AddRenderPass(resolvePass->Autorelease());
			}
			else
			{
				_previewCamera->GetRenderPass()->SetFramebuffer(_previewWindow->GetFramebuffer());
			}

			world->AddNode(_previewCamera->Autorelease());
			
			if(!_vrWindow)
			{
				_headCamera = _previewCamera;
			}
		}
	}

	void CameraManager::RegeneratePipeline()
	{
		ClearPipeline();
		GeneratePipeline();
	}
}
