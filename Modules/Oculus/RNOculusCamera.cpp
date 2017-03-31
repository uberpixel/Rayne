//
//  RNOculusCamera.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"
#include "RNOculusCamera.h"

namespace RN
{
	RNDefineMeta(OculusCamera, SceneNode)

	OculusCamera::OculusCamera(bool debug) :
		_window(new OculusWindow()),
		_isDebug(debug),
		_debugWindow(nullptr),
		_head(new SceneNode())
	{
		Vector2 windowSize = _window->GetSize();

		if(_isDebug)
		{
			_debugWindow = Renderer::GetActiveRenderer()->CreateAWindow(windowSize, Screen::GetMainScreen());
			_debugWindow->SetTitle(RNCSTR("Oculus Debug"));
			_debugWindow->Show();
		}

		AddChild(_head);

		for(int i = 0; i < 2; i++)
		{
			_eye[i] = new Camera();

			if(!_isDebug)
				_eye[i]->SetFramebuffer(_window->GetFramebuffer());
			else
				_eye[i]->SetFramebuffer(_debugWindow->GetFramebuffer());

			_eye[i]->SetFrame(Rect(i * windowSize.x / 2, 0, windowSize.x / 2, windowSize.y));
			_head->AddChild(_eye[i]);
		}
	}

	OculusCamera::~OculusCamera()
	{
		_window->Release();
		_head->Release();
		_eye[0]->Release();
		_eye[1]->Release();
	}

	void OculusCamera::Update(float delta)
	{
		_window->UpdateTrackingData(_eye[0]->GetClipNear(), _eye[0]->GetClipFar());
		const OculusHMDTrackingState &hmdState = _window->GetHMDTrackingState();

		_eye[0]->SetPosition(hmdState.eyeOffset[0]);
		_eye[1]->SetPosition(hmdState.eyeOffset[1]);
		_eye[0]->SetProjectionMatrix(hmdState.eyeProjection[0]);
		_eye[1]->SetProjectionMatrix(hmdState.eyeProjection[1]);

		_head->SetRotation(hmdState.rotation);
		_head->SetPosition(hmdState.position);
	}

	const OculusHMDTrackingState &OculusCamera::GetHMDTrackingState()
	{
		return _window->GetHMDTrackingState();
	}

	const OculusTouchTrackingState &OculusCamera::GetTouchTrackingState(int hand)
	{
		return _window->GetTouchTrackingState(hand);
	}

	void OculusCamera::SubmitTouchHaptics(int hand, const OculusTouchHaptics &haptics)
	{
		_window->SubmitTouchHaptics(hand, haptics);
	}
}
