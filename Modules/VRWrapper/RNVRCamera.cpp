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
		_previewRenderPass(previewRenderPass? previewRenderPass->Retain() : nullptr)
	{
		Vector2 windowSize = _window->GetSize();
		if(_debugWindow)
		{
			windowSize = _debugWindow->GetSize();
			_debugWindow->SetTitle(RNCSTR("VR Debug Window"));
			_debugWindow->Show();
		}

		AddChild(_head);

		//TODO: Maybe handle different resolutions per eye
		Vector2 eyeSize((windowSize.x - _window->GetEyePadding()) / 2, windowSize.y);
		Framebuffer *msaaFramebuffer = nullptr;
		Framebuffer *resolvedFramebuffer = nullptr;
		PostProcessingAPIStage *resolvePass[2];
		PostProcessingAPIStage *copyPass[2];

		Texture *resolvedTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormat(Texture::Format::RGBA8888SRGB, eyeSize.x, eyeSize.y));
		resolvedFramebuffer = Renderer::GetActiveRenderer()->CreateFramebuffer(eyeSize);
		resolvedFramebuffer->SetColorTarget(Framebuffer::TargetView::WithTexture(resolvedTexture->Autorelease()));

		if(msaaSampleCount > 1)
		{
			Texture *msaaTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormatAndMSAA(Texture::Format::RGBA8888SRGB, eyeSize.x, eyeSize.y, 8));
			Texture *msaaDepthTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormatAndMSAA(Texture::Format::Depth24Stencil8, eyeSize.x, eyeSize.y, 8));
			msaaFramebuffer = Renderer::GetActiveRenderer()->CreateFramebuffer(eyeSize);
			msaaFramebuffer->SetColorTarget(Framebuffer::TargetView::WithTexture(msaaTexture->Autorelease()));
			msaaFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(msaaDepthTexture->Autorelease()));
		}
		else
		{
			Texture *resolvedDepthTexture = Texture::WithDescriptor(Texture::Descriptor::With2DRenderTargetFormat(Texture::Format::Depth24Stencil8, eyeSize.x, eyeSize.y));
			resolvedFramebuffer->SetDepthStencilTarget(Framebuffer::TargetView::WithTexture(resolvedDepthTexture->Autorelease()));
		}
		

		for(int i = 0; i < 2; i++)
		{
			_eye[i] = new Camera();
			_head->AddChild(_eye[i]);

			copyPass[i] = new PostProcessingAPIStage(PostProcessingAPIStage::Type::CopyBuffer);
			copyPass[i]->SetFramebuffer(_debugWindow ? _debugWindow->GetFramebuffer() : _window->GetFramebuffer());
			copyPass[i]->SetFrame(Rect(i * (windowSize.x + _window->GetEyePadding()) / 2, 0, (windowSize.x - _window->GetEyePadding()) / 2, windowSize.y));

			if(msaaSampleCount > 1)
			{
				resolvePass[i] = new PostProcessingAPIStage(PostProcessingAPIStage::Type::ResolveMSAA);
				resolvePass[i]->SetFramebuffer(resolvedFramebuffer->Autorelease());
				resolvePass[i]->AddRenderPass(copyPass[i]->Autorelease());

				_eye[i]->GetRenderPass()->SetFramebuffer(msaaFramebuffer);
				_eye[i]->GetRenderPass()->AddRenderPass(resolvePass[i]);
			}
			else
			{
				_eye[i]->GetRenderPass()->SetFramebuffer(resolvedFramebuffer);
				_eye[i]->GetRenderPass()->AddRenderPass(copyPass[i]);
			}
		}

		if(_previewRenderPass)
		{
			if (msaaSampleCount > 1)
			{
				resolvePass[0]->AddRenderPass(_previewRenderPass);
			}
			else
			{
				_eye[0]->GetRenderPass()->AddRenderPass(_previewRenderPass);
			}
		}
	}

	VRCamera::~VRCamera()
	{
		SafeRelease(_previewRenderPass);
		SafeRelease(_window);
		SafeRelease(_debugWindow);
		SafeRelease(_head);
		SafeRelease(_eye[0]);
		SafeRelease(_eye[1]);
	}

	void VRCamera::Update(float delta)
	{
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

	const VRControllerTrackingState &VRCamera::GetControllerTrackingState(int hand) const
	{
		return _window->GetControllerTrackingState(hand);
	}

	void VRCamera::SubmitControllerHaptics(int hand, const VRControllerHaptics &haptics) const
	{
		_window->SubmitControllerHaptics(hand, haptics);
	}
}
