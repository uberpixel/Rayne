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

		VRCamera::VRCamera(VRWindow *window, Window *debugWindow) :
		_window(window->Retain()),
		_debugWindow(debugWindow?debugWindow->Retain():nullptr),
		_head(new SceneNode())
	{
		Vector2 windowSize = _window->GetSize();
		if(_debugWindow)
		{
			windowSize = _debugWindow->GetSize();
			_debugWindow->SetTitle(RNCSTR("VR Debug Window"));
			_debugWindow->Show();
		}

		AddChild(_head);

		for(int i = 0; i < 2; i++)
		{
			_eye[i] = new Camera();
			_eye[i]->SetFramebuffer(_debugWindow? _debugWindow->GetFramebuffer() : _window->GetFramebuffer());

			_eye[i]->SetFrame(Rect(i * windowSize.x / 2, 0, windowSize.x / 2, windowSize.y));
			_head->AddChild(_eye[i]);
		}
	}

	VRCamera::~VRCamera()
	{
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
