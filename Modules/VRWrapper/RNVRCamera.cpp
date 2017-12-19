//
//  RNVRCamera.cpp
//  Rayne-VR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVRCamera.h"
#include "../../Source/Rendering/RNPostProcessing.h"

namespace RN
{
	RNDefineMeta(VRCamera, SceneNode)

		VRCamera::VRCamera(VRWindow *window, RenderPass *previewRenderPass, uint8 msaaSampleCount, Window *debugWindow) :
		_window(window->Retain()),
		_debugWindow(debugWindow?debugWindow->Retain():nullptr),
		_head(new SceneNode()),
		_previewRenderPass(previewRenderPass? previewRenderPass->Retain() : nullptr),
		_msaaSampleCount(msaaSampleCount),
		_eye{nullptr, nullptr}
	{
		AddChild(_head);
		SetupCameras();
	}

	VRCamera::~VRCamera()
	{
		NotificationManager::GetSharedInstance()->RemoveSubscriber(kRNWindowDidChangeSize, this);

		SafeRelease(_previewRenderPass);
		SafeRelease(_window);
		SafeRelease(_debugWindow);
		SafeRelease(_head);
		SafeRelease(_eye[0]);
		SafeRelease(_eye[1]);
	}
	
	void VRCamera::SetupCameras()
	{
		if(!_window->IsRendering() && !_debugWindow)
			return;
		
		Vector2 windowSize = _window->GetSize();
		if(_debugWindow)
		{
			windowSize = _debugWindow->GetSize();
			_debugWindow->SetTitle(RNCSTR("VR Debug Window"));
			_debugWindow->Show();
		}
		
		//TODO: Maybe handle different resolutions per eye
		Vector2 eyeSize((windowSize.x - _window->GetEyePadding()) / 2, windowSize.y);
		
		for(int i = 0; i < 2; i++)
		{
			_eye[i] = new Camera();
			_eye[i]->SetRenderGroup(0x1 | (1 << (1 + i)));
			_head->AddChild(_eye[i]);
			_hiddenAreaEntity[i] = nullptr;
			
#if !RN_PLATFORM_WINDOWS
			Mesh *hiddenAreaMesh = _window->GetHiddenAreaMesh(i);
			if(hiddenAreaMesh)
			{
				ShaderLibrary *shaderLibrary = RN::Renderer::GetActiveRenderer()->GetDefaultShaderLibrary();
				Material *hiddenAreaMaterial = Material::WithShaders(shaderLibrary->GetShaderWithName(RNCSTR("pp_mask_vertex"), Shader::Options::WithMesh(hiddenAreaMesh)), shaderLibrary->GetShaderWithName(RNCSTR("pp_mask_fragment")));
				hiddenAreaMaterial->SetCullMode(CullMode::None);
				hiddenAreaMaterial->SetColorWriteMask(false, false, false, false);
				
				Model *hiddenAreaModel = new Model(hiddenAreaMesh, hiddenAreaMaterial);
				_hiddenAreaEntity[i] = new Entity(hiddenAreaModel->Autorelease());
				_hiddenAreaEntity[i]->SetPriority(RN::SceneNode::Priority::UpdateEarly);
				_hiddenAreaEntity[i]->SetRenderGroup((1 << (1 + i)));
				
				_eye[i]->AddChild(_hiddenAreaEntity[i]);
			}
#endif
		}

#if !RN_PLATFORM_MAC_OS
		_eye[0]->GetRenderPass()->SetFlags(0);
#endif
		
		CreatePostprocessingPipeline();
		
		NotificationManager::GetSharedInstance()->AddSubscriber(kRNWindowDidChangeSize, [this](Notification *notification) {
			if(notification->GetName()->IsEqual(kRNWindowDidChangeSize) && notification->GetInfo<RN::VRWindow>() == _window)
			{
				CreatePostprocessingPipeline();
			}
		}, this);
	}

	void VRCamera::CreatePostprocessingPipeline()
	{
		Vector2 windowSize = _window->GetSize();

		//TODO: Maybe handle different resolutions per eye
		Vector2 eyeSize((windowSize.x - _window->GetEyePadding()) / 2, windowSize.y);

		Framebuffer *msaaFramebuffer = nullptr;
		Framebuffer *resolvedFramebuffer = _debugWindow ? _debugWindow->GetFramebuffer() : _window->GetFramebuffer();
		PostProcessingAPIStage *resolvePass = nullptr;
		
		Texture::Format colorFormat = _window->GetSwapChainDescriptor().colorFormat;
		Texture::Format depthFormat = _window->GetSwapChainDescriptor().depthStencilFormat;
		if(depthFormat == Texture::Format::Invalid)
		{
			depthFormat = Texture::Format::Depth24Stencil8;
		}

		if(_msaaSampleCount > 1)
		{
			Texture *msaaTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormatAndMSAA(colorFormat, windowSize.x, windowSize.y, _msaaSampleCount));
			Texture *msaaDepthTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormatAndMSAA(depthFormat, windowSize.x, windowSize.y, _msaaSampleCount));
			msaaFramebuffer = Renderer::GetActiveRenderer()->CreateFramebuffer(windowSize);
			msaaFramebuffer->SetColorTarget(Framebuffer::TargetView::WithTexture(msaaTexture));
			msaaFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(msaaDepthTexture));
		}

		//TODO: Maybe instead of checking depthStencilFormat, check for oculus window?
		if(_msaaSampleCount <= 1 && (_window->GetSwapChainDescriptor().depthStencilFormat == Texture::Format::Invalid || _debugWindow))
		{
			Texture *resolvedDepthTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormat(depthFormat, windowSize.x, windowSize.y));
			resolvedFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(resolvedDepthTexture));
		}
		
		for(int i = 0; i < 2; i++)
		{
			_eye[i]->GetRenderPass()->RemoveAllRenderPasses();
			_eye[i]->GetRenderPass()->SetFrame(Rect(i * (windowSize.x + _window->GetEyePadding()) / 2, 0, (windowSize.x - _window->GetEyePadding()) / 2, windowSize.y));

			if(_msaaSampleCount > 1)
			{
				_eye[i]->GetRenderPass()->SetFramebuffer(msaaFramebuffer);
			}
			else
			{
				_eye[i]->GetRenderPass()->SetFramebuffer(resolvedFramebuffer);
			}
		}

		if(_msaaSampleCount > 1)
		{
			resolvePass = new PostProcessingAPIStage(PostProcessingAPIStage::Type::ResolveMSAA);
			resolvePass->SetFramebuffer(resolvedFramebuffer);
			_eye[0]->GetRenderPass()->AddRenderPass(resolvePass);
		}

		if(_previewRenderPass)
		{
			if(resolvePass)
			{
				resolvePass->AddRenderPass(_previewRenderPass);
			}
			else
			{
				_eye[0]->GetRenderPass()->AddRenderPass(_previewRenderPass);
			}
		}
	}

	void VRCamera::Update(float delta)
	{
		if(!_eye[0] || !_eye[1])
			return;
		
		_window->Update(delta, _eye[0]->GetClipNear(), _eye[0]->GetClipFar());
		const VRHMDTrackingState &hmdState = GetHMDTrackingState();

		_eye[0]->SetPosition(hmdState.eyeOffset[0]);
		_eye[1]->SetPosition(hmdState.eyeOffset[1]);
		_eye[0]->SetProjectionMatrix(hmdState.eyeProjection[0]);
		_eye[1]->SetProjectionMatrix(hmdState.eyeProjection[1]);

		_head->SetRotation(hmdState.rotation);
		_head->SetPosition(hmdState.position);
	}

	const VRHMDTrackingState &VRCamera::GetHMDTrackingState() const
	{
		return _window->GetHMDTrackingState();
	}

	const VRControllerTrackingState &VRCamera::GetControllerTrackingState(uint8 index) const
	{
		return _window->GetControllerTrackingState(index);
	}

	const VRControllerTrackingState &VRCamera::GetTrackerTrackingState(uint8 index) const
	{
		return _window->GetTrackerTrackingState(index);
	}

	void VRCamera::SubmitControllerHaptics(uint8 controllerID, const VRControllerHaptics &haptics) const
	{
		_window->SubmitControllerHaptics(controllerID, haptics);
	}
	
	void VRCamera::SetClipFar(float clipFar)
	{
		if(_eye[0])
		{
			_eye[0]->SetClipFar(clipFar);
		}
		
		if(_eye[1])
		{
			_eye[1]->SetClipFar(clipFar);
		}
	}
	
	void VRCamera::SetClipNear(float clipNear)
	{
		if(_eye[0])
		{
			_eye[0]->SetClipNear(clipNear);
		}
		
		if(_eye[1])
		{
			_eye[1]->SetClipNear(clipNear);
		}
	}
}
