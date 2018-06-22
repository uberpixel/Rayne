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
		_window(window? window->Retain() : nullptr),
		_debugWindow(debugWindow? debugWindow->Retain() : nullptr),
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
		if(!_window && !_debugWindow) return;

		if(_window && !_window->IsRendering()) return;
		
		Vector2 windowSize;
		float eyePadding = 0.0f;

		if(_window)
		{
			windowSize = _window->GetSize();
			eyePadding = _window->GetEyePadding();
		}

		if(_debugWindow)
		{
			windowSize = _debugWindow->GetSize();
			_debugWindow->SetTitle(RNCSTR("VR Debug Window"));
			_debugWindow->Show();
		}
		
		for(int i = 0; i < 2; i++)
		{
			_eye[i] = new Camera();
			_eye[i]->SetRenderGroup(0x1 | (1 << (1 + i)));
			_head->AddChild(_eye[i]);
			_hiddenAreaEntity[i] = nullptr;
			
#if !RN_PLATFORM_WINDOWS
			if(_window)
			{
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
			}
#endif
		}

		_eye[0]->SetPosition(Vector3(-0.032f, 0.0f, 0.0f));
        _eye[1]->SetPosition(Vector3(0.032f, 0.0f, 0.0f));

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
		Framebuffer *resolvedFramebuffer = _debugWindow ? _debugWindow->GetFramebuffer() : _window->GetFramebuffer();
		RN_ASSERT(resolvedFramebuffer, "The VRWindow has no framebuffer!");

		Vector2 windowSize;
		float eyePadding = 0.0f;
		Texture::Format colorFormat = Texture::Format::RGBA8888;
        Texture::Format depthFormat = Texture::Format::Invalid;

		if(_window)
		{
			windowSize = _window->GetSize();
			eyePadding = _window->GetEyePadding();

			colorFormat = _window->GetSwapChainDescriptor().colorFormat;
            depthFormat = _window->GetSwapChainDescriptor().depthStencilFormat;
		}

		if(_debugWindow)
		{
			windowSize = _debugWindow->GetSize();
			RNDebug(RNCSTR("Window size: ") << windowSize.x << RNCSTR(" x ") << windowSize.y);
		}

		Framebuffer *msaaFramebuffer = nullptr;
		PostProcessingAPIStage *resolvePass = nullptr;

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
		if(_msaaSampleCount <= 1 && (!_window || _window->GetSwapChainDescriptor().depthStencilFormat == Texture::Format::Invalid || _debugWindow))
		{
			Texture *resolvedDepthTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormat(depthFormat, windowSize.x, windowSize.y));
			resolvedFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(resolvedDepthTexture));
		}
		
		for(int i = 0; i < 2; i++)
		{
			_eye[i]->GetRenderPass()->RemoveAllRenderPasses();
			_eye[i]->GetRenderPass()->SetFrame(Rect(i * (windowSize.x + eyePadding) / 2, 0, (windowSize.x - eyePadding) / 2, windowSize.y));

#if RN_PLATFORM_ANDROID
			if(_debugWindow)
				_eye[i]->GetRenderPass()->SetFrame(Rect(0.0f, i * (windowSize.y + eyePadding) / 2, windowSize.x, (windowSize.y - eyePadding) / 2));
#endif

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
		SceneNode::Update(delta);

		if(!_window || !_eye[0] || !_eye[1]) return;
		
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
