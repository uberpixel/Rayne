//
//  RNOculusCamera.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"
#include "RNOculusCamera.h"
#include "RNOculusWindow.h"

namespace RN
{
	RNDefineMeta(OculusCamera, SceneNode)

	OculusCamera::OculusCamera(bool debug) :
		_window(new OculusWindow()),
		_isDebug(debug),
		_debugWindow(nullptr)
	{
		Vector2 windowSize = _window->GetSize();

		if(_isDebug)
		{
			_debugWindow = Renderer::GetActiveRenderer()->CreateAWindow(windowSize, Screen::GetMainScreen());
			_debugWindow->SetTitle(RNCSTR("Oculus Debug"));
			_debugWindow->Show();
		}

		for(int i = 0; i < 2; i++)
		{
			_camera[i] = new Camera();

			if(!_isDebug)
				_camera[i]->SetFramebuffer(_window->GetFramebuffer());
			else
				_camera[i]->SetFramebuffer(_debugWindow->GetFramebuffer());

			_camera[i]->SetFrame(Rect(i * windowSize.x / 2, 0, windowSize.x / 2, windowSize.y));
			AddChild(_camera[i]);
		}
	}

	OculusCamera::~OculusCamera()
	{
		_window->Release();
		_camera[0]->Release();
		_camera[1]->Release();
	}

	void OculusCamera::Update(float delta)
	{
		_window->UpdateTrackingData();
		//_camera->SetRotation(_oculusWindow->GetHeadRotation());
		for(int i = 0; i < 2; i++)
		{
			_camera[i]->SetProjectionMatrix(_window->GetProjectionMatrix(i, _camera[i]->GetClipNear(), _camera[i]->GetClipFar()));
			_camera[i]->SetPosition(_window->GetEyePosition(i));
			_camera[i]->SetRotation(_window->GetEyeRotation(i));
		}
	}
}
