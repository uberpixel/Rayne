//
//  RNVRCamera.cpp
//  Rayne-VR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVRCamera.h"

namespace RN
{
	RNDefineMeta(VRCamera, SceneNode)

		VRCamera::VRCamera(VRWindow *window, RenderPass *previewRenderPass, uint8 msaaSampleCount, Window *debugWindow) :
		_window(window? window->Retain() : nullptr),
		_debugWindow(debugWindow? debugWindow->Retain() : nullptr),
		_head(new Camera()),
		_previewRenderPass(previewRenderPass? previewRenderPass->Retain() : nullptr),
		_msaaSampleCount(msaaSampleCount),
		_eye{nullptr, nullptr},
		_didUpdateVRWindow(false)
	{
		SetUpdatePriority(SceneNode::UpdatePriority::UpdateEarly);
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

		if(_debugWindow)
		{
			_debugWindow->SetTitle(RNCSTR("VR Debug Window"));
			_debugWindow->Show();
		}
		
		_head->AddFlags(Camera::Flags::UseSimpleCulling);
		_head->SetShaderHint(Shader::UsageHint::Multiview);
		_head->SetFOV(110.0f);
		
		for(int i = 0; i < 2; i++)
		{
			_eye[i] = new Camera();
			//_eye[i]->SetRenderGroup(0x1 | (1 << (1 + i))); //This won't work with multiview! (and is only needed for the hidden area mask)
			_head->AddChild(_eye[i]);
			_head->AddMultiviewCamera(_eye[i]);
			_hiddenAreaEntity[i] = nullptr;

			//TODO: Fix culling for VR!?
			_eye[i]->AddFlags(Camera::Flags::UseSimpleCulling);
			
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
					_hiddenAreaEntity[i]->SetUpdatePriority(RN::SceneNode::UpdatePriority::UpdateEarly);
					_hiddenAreaEntity[i]->SetRenderGroup((1 << (1 + i)));

					_eye[i]->AddChild(_hiddenAreaEntity[i]);
				}
			}
#endif
		}

		_eye[0]->SetPosition(Vector3(-0.032f, 0.0f, 0.0f));
		_eye[1]->SetPosition(Vector3(0.032f, 0.0f, 0.0f));

#if RN_PLATFORM_ANDROID

#else
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
		Framebuffer *resolvedFramebuffer = _window->GetFramebuffer();
		RN_ASSERT(resolvedFramebuffer, "The VRWindow has no framebuffer!");

		Vector2 windowSize;
		Texture::Format colorFormat = Texture::Format::RGBA_8_SRGB;
		Texture::Format depthFormat = Texture::Format::Invalid;
		uint8 layerCount = 1;

		if(_window)
		{
			windowSize = _window->GetSize();

			colorFormat = _window->GetSwapChainDescriptor().colorFormat;
			depthFormat = _window->GetSwapChainDescriptor().depthStencilFormat;

			layerCount = _window->GetSwapChainDescriptor().layerCount;
		}

		PostProcessingStage *sideBySideDebugPass = nullptr;
		if(_debugWindow)
		{
			//windowSize = _debugWindow->GetSize();

			//colorFormat = _debugWindow->GetSwapChainDescriptor().colorFormat;
			//depthFormat = _debugWindow->GetSwapChainDescriptor().depthStencilFormat;

			sideBySideDebugPass = new PostProcessingStage();
			Material *copyMultiviewToSideBySideDebugMaterial = Material::WithShaders(Renderer::GetActiveRenderer()->GetDefaultShaderLibrary()->GetShaderWithName(RNCSTR("pp_vertex")), Renderer::GetActiveRenderer()->GetDefaultShaderLibrary()->GetShaderWithName(RNCSTR("pp_blit_fragment"), Shader::Options::WithNone()->AddDefine(RNCSTR("RN_PP_VR"), RNCSTR("1"))));
			sideBySideDebugPass->SetFramebuffer(_debugWindow->GetFramebuffer());
			sideBySideDebugPass->SetMaterial(copyMultiviewToSideBySideDebugMaterial);
			sideBySideDebugPass->Autorelease();
		}

		Framebuffer *msaaFramebuffer = nullptr;
		PostProcessingAPIStage *resolvePass = nullptr;

		if(depthFormat == Texture::Format::Invalid)
		{
			depthFormat = Texture::Format::Depth_32F;
		}

		if(_msaaSampleCount > 1)
		{
			Texture::Descriptor msaaColorTextureDescriptor = Texture::Descriptor::With2DRenderTargetFormatAndMSAA(colorFormat, windowSize.x, windowSize.y, _msaaSampleCount);
			msaaColorTextureDescriptor.depth = layerCount;
			msaaColorTextureDescriptor.type = layerCount > 1? Texture::Type::Type2DArray : Texture::Type::Type2D;
			Texture *msaaTexture = Texture::WithDescriptor(msaaColorTextureDescriptor);

			Texture::Descriptor msaaDepthTextureDescriptor = Texture::Descriptor::With2DRenderTargetFormatAndMSAA(depthFormat, windowSize.x, windowSize.y, _msaaSampleCount);
			msaaDepthTextureDescriptor.depth = layerCount;
			msaaDepthTextureDescriptor.type = layerCount > 1? Texture::Type::Type2DArray : Texture::Type::Type2D;
			Texture *msaaDepthTexture = Texture::WithDescriptor(msaaDepthTextureDescriptor);

			msaaFramebuffer = Renderer::GetActiveRenderer()->CreateFramebuffer(windowSize);
			msaaFramebuffer->SetColorTarget(Framebuffer::TargetView::WithTexture(msaaTexture));
			msaaFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(msaaDepthTexture));
		}

		//TODO: Depth buffer handling for Android with 2 resolve buffers
		if(_msaaSampleCount <= 1 && (!_window || _window->GetSwapChainDescriptor().depthStencilFormat == Texture::Format::Invalid || _debugWindow))
		{
			Texture::Descriptor depthTextureDescriptor = Texture::Descriptor::With2DRenderTargetFormat(depthFormat, windowSize.x, windowSize.y);
			depthTextureDescriptor.depth = layerCount;
			depthTextureDescriptor.type = layerCount > 1 ? Texture::Type::Type2DArray : Texture::Type::Type2D;
			
			Texture *resolvedDepthTexture = Texture::WithDescriptor(depthTextureDescriptor);
			resolvedFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(resolvedDepthTexture));
		}

		if(_msaaSampleCount > 1)
		{
			resolvePass = new PostProcessingAPIStage(PostProcessingAPIStage::Type::ResolveMSAA);
			resolvePass->SetFramebuffer(resolvedFramebuffer);

			_head->GetRenderPass()->SetFramebuffer(msaaFramebuffer);
			_head->GetRenderPass()->AddRenderPass(resolvePass);
		}
		else
		{
			_head->GetRenderPass()->SetFramebuffer(resolvedFramebuffer);
		}

		if(sideBySideDebugPass)
		{
			if(resolvePass)
			{
				resolvePass->AddRenderPass(sideBySideDebugPass);
			}
			else
			{
				_head->GetRenderPass()->AddRenderPass(sideBySideDebugPass);
			}
		}

		if(_previewRenderPass)
		{
			if(resolvePass)
			{
				resolvePass->AddRenderPass(_previewRenderPass);
			}
			else
			{
				_head->GetRenderPass()->AddRenderPass(_previewRenderPass);
			}
		}
	}

	void VRCamera::UpdateVRWindow(float delta)
	{
		if(_didUpdateVRWindow || !_window || !_eye[0] || !_eye[1]) return;
		_window->Update(delta, _eye[0]->GetClipNear(), _eye[0]->GetClipFar());
		_didUpdateVRWindow = true;
	}

	void VRCamera::Update(float delta)
	{
		SceneNode::Update(delta);

		if(!_window || !_eye[0] || !_eye[1]) return;
		
		UpdateVRWindow(delta);
		const VRHMDTrackingState &hmdState = GetHMDTrackingState();

		_eye[0]->SetPosition(hmdState.eyeOffset[0]);
		_eye[1]->SetPosition(hmdState.eyeOffset[1]);
		_eye[0]->SetProjectionMatrix(hmdState.eyeProjection[0]);
		_eye[1]->SetProjectionMatrix(hmdState.eyeProjection[1]);

		_head->SetRotation(hmdState.rotation);
		_head->SetPosition(hmdState.position);
		
		_didUpdateVRWindow = false;
	}

	VRHMDTrackingState VRCamera::GetHMDTrackingState() const
	{
		VRHMDTrackingState trackingState = _window->GetHMDTrackingState();
		trackingState.position += _originPositionOffset;
		trackingState.position = _originalOrientationOffset.GetRotatedVector(trackingState.position);
		trackingState.rotation = _originalOrientationOffset * trackingState.rotation;
		
		return trackingState;
	}

	VRControllerTrackingState VRCamera::GetControllerTrackingState(uint8 index) const
	{
		VRControllerTrackingState trackingState = _window->GetControllerTrackingState(index);
		trackingState.positionAim += _originPositionOffset;
		trackingState.positionAim = _originalOrientationOffset.GetRotatedVector(trackingState.positionAim);
		trackingState.rotationAim = _originalOrientationOffset * trackingState.rotationAim;
		trackingState.positionGrip += _originPositionOffset;
		trackingState.positionGrip = _originalOrientationOffset.GetRotatedVector(trackingState.positionGrip);
		trackingState.rotationGrip = _originalOrientationOffset * trackingState.rotationGrip;
		
		trackingState.velocityLinear = _originalOrientationOffset.GetRotatedVector(trackingState.velocityLinear);
		
		return trackingState;
	}

	VRControllerTrackingState VRCamera::GetTrackerTrackingState(uint8 index) const
	{
		VRControllerTrackingState trackingState = _window->GetTrackerTrackingState(index);
		trackingState.positionAim += _originPositionOffset;
		trackingState.positionAim = _originalOrientationOffset.GetRotatedVector(trackingState.positionAim);
		trackingState.rotationAim = _originalOrientationOffset * trackingState.rotationAim;
		trackingState.positionGrip += _originPositionOffset;
		trackingState.positionGrip = _originalOrientationOffset.GetRotatedVector(trackingState.positionGrip);
		trackingState.rotationGrip = _originalOrientationOffset * trackingState.rotationGrip;
		
		trackingState.velocityLinear = _originalOrientationOffset.GetRotatedVector(trackingState.velocityLinear);
		
		return trackingState;
	}

	VRHandTrackingState VRCamera::GetHandTrackingState(uint8 index) const
	{
		VRHandTrackingState trackingState = _window->GetHandTrackingState(index);
		trackingState.position += _originPositionOffset;
		trackingState.position = _originalOrientationOffset.GetRotatedVector(trackingState.position);
		trackingState.rotation = _originalOrientationOffset * trackingState.rotation;
		
		return trackingState;
	}

	void VRCamera::SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics) const
	{
		_window->SubmitControllerHaptics(index, haptics);
	}

	const VRWindow::Origin VRCamera::GetOrigin() const
	{
		return _window->GetOrigin();
	}
	
	void VRCamera::SetClipFar(float clipFar)
	{
		_head->SetClipFar(clipFar);
		
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
		_head->SetClipNear(clipNear);
		
		if(_eye[0])
		{
			_eye[0]->SetClipNear(clipNear);
		}
		
		if(_eye[1])
		{
			_eye[1]->SetClipNear(clipNear);
		}
	}
	
	void VRCamera::SetOriginOffset(const Vector3 &positionOffset, const Quaternion &orientationOffset)
	{
		_originPositionOffset = positionOffset;
		_originalOrientationOffset = orientationOffset;
	}
}
