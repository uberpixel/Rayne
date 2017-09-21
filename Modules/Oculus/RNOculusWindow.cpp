//
//  RNOculusWindow.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"
#include "RNOculusWindow.h"

#include "OVR_CAPI_Audio.h"
#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

namespace RN
{
	RNDefineMeta(OculusWindow, VRWindow)

	OculusWindow::OculusWindow() : _swapChain(nullptr)
	{
		
	}

	OculusWindow::~OculusWindow()
	{
		StopRendering();
	}

	void OculusWindow::StartRendering(const SwapChainDescriptor &descriptor)
	{
		_swapChain = new OculusSwapChain(descriptor);
	}

	void OculusWindow::StopRendering()
	{
		SafeRelease(_swapChain);
	}

	Vector2 OculusWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *OculusWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	uint32 OculusWindow::GetEyePadding() const
	{
		return OculusSwapChain::kEyePadding;
	}

	static Vector3 GetVectorForOVRVector(const ovrVector3f &ovrVector)
	{
		Vector3 result;
		result.x = ovrVector.x;
		result.y = ovrVector.y;
		result.z = ovrVector.z;
		return result;
	}

	static Vector2 GetVectorForOVRVector(const ovrVector2f &ovrVector)
	{
		Vector2 result;
		result.x = ovrVector.x;
		result.y = ovrVector.y;
		return result;
	}

	static Quaternion GetQuaternionForOVRQuaternion(const ovrQuatf &ovrQuat)
	{
		Quaternion result;
		result.x = ovrQuat.x;
		result.y = ovrQuat.y;
		result.z = ovrQuat.z;
		result.w = ovrQuat.w;
		return result;
	}

	static Matrix GetMatrixForOVRMatrix(const ovrMatrix4f &ovrMatrix)
	{
		Matrix result;
		for(int q = 0; q < 4; q++)
		{
			for(int t = 0; t < 4; t++)
			{
				result.m[q * 4 + t] = ovrMatrix.M[t][q];
			}
		}
		return result;
	}

	void OculusWindow::Update(float delta, float near, float far)
	{
		_swapChain->UpdatePredictedPose();

		_hmdTrackingState.eyeOffset[0] = GetVectorForOVRVector(_swapChain->_hmdToEyeViewPose[0].Position);
		_hmdTrackingState.eyeOffset[1] = GetVectorForOVRVector(_swapChain->_hmdToEyeViewPose[1].Position);
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(ovrMatrix4f_Projection(_swapChain->_layer.Fov[0], near, far, ovrProjection_None));
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(ovrMatrix4f_Projection(_swapChain->_layer.Fov[1], near, far, ovrProjection_None));
		_hmdTrackingState.position = GetVectorForOVRVector(_swapChain->_hmdState.HeadPose.ThePose.Position);
		_hmdTrackingState.rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HeadPose.ThePose.Orientation);

		if(_swapChain->_submitResult == ovrSuccess_NotVisible)
		{
			//Check if application should quit! Stop most things.
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Paused;
		}
		else if(_swapChain->_submitResult == ovrError_DisplayLost)
		{
			//Recreate session/swap chain or quit application with an error message
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
		}
		else if(OVR_FAILURE(_swapChain->_submitResult))
		{
			//Quit the application
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
		}
		else
		{
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Rendering;
		}

		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(_swapChain->_session, &sessionStatus);

		if(sessionStatus.ShouldQuit)
		{
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
		}
		if(sessionStatus.ShouldRecenter)
		{
			ovr_RecenterTrackingOrigin(_swapChain->_session); // or ovr_ClearShouldRecenterFlag(_swapChain->_session) to ignore the request.
		}


		ovrInputState inputState;
		if(OVR_SUCCESS(ovr_GetInputState(_swapChain->_session, ovrControllerType_Touch, &inputState)))
		{
			_controllerTrackingState[0].active = (inputState.ControllerType & ovrControllerType_LTouch);
			_controllerTrackingState[0].position = GetVectorForOVRVector(_swapChain->_hmdState.HandPoses[0].ThePose.Position);
			_controllerTrackingState[0].rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HandPoses[0].ThePose.Orientation);
			_controllerTrackingState[0].rotation *= RN::Vector3(0.0f, 45.0f, 0.0f);
			_controllerTrackingState[0].thumbstick = GetVectorForOVRVector(inputState.Thumbstick[0]);
			_controllerTrackingState[0].indexTrigger = inputState.IndexTrigger[0];
			_controllerTrackingState[0].handTrigger = inputState.HandTrigger[0];

			_controllerTrackingState[1].active = (inputState.ControllerType & ovrControllerType_RTouch);
			_controllerTrackingState[1].position = GetVectorForOVRVector(_swapChain->_hmdState.HandPoses[1].ThePose.Position);
			_controllerTrackingState[1].rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HandPoses[1].ThePose.Orientation);
			_controllerTrackingState[1].rotation *= RN::Vector3(0.0f, 45.0f, 0.0f);
			_controllerTrackingState[1].thumbstick = GetVectorForOVRVector(inputState.Thumbstick[1]);
			_controllerTrackingState[1].indexTrigger = inputState.IndexTrigger[1];
			_controllerTrackingState[1].handTrigger = inputState.HandTrigger[1];

			_controllerTrackingState[0].button[VRControllerTrackingState::Button::AX] = inputState.Buttons & ovrButton_X;
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::BY] = inputState.Buttons & ovrButton_Y;
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::Stick] = inputState.Buttons & ovrButton_LThumb;
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::Start] = inputState.Buttons & ovrButton_Enter;

			_controllerTrackingState[1].button[VRControllerTrackingState::Button::AX] = inputState.Buttons & ovrButton_A;
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::BY] = inputState.Buttons & ovrButton_B;
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::Stick] = inputState.Buttons & ovrButton_RThumb;
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::Start] = false;
		}
		else
		{
			_controllerTrackingState[0].active = false;
			_controllerTrackingState[1].active = false;
		}
	}

	const VRHMDTrackingState &OculusWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const VRControllerTrackingState &OculusWindow::GetControllerTrackingState(int hand) const
	{
		return _controllerTrackingState[hand];
	}

	void OculusWindow::SubmitControllerHaptics(int hand, const VRControllerHaptics &haptics)
	{
		ovrHapticsBuffer buffer;
		buffer.SubmitMode = ovrHapticsBufferSubmit_Enqueue;
		buffer.SamplesCount = haptics.sampleCount;
		buffer.Samples = haptics.samples;
		ovr_SubmitControllerVibration(_swapChain->_session, hand?ovrControllerType_RTouch:ovrControllerType_LTouch, &buffer);
	}

	static String *StringForLPWSTR(LPWSTR lpwstr)
	{
		int length = WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, nullptr, 0, nullptr, nullptr);
		char *chars = new char[length+1];
		WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, chars, length, nullptr, nullptr);
		chars[length] = '\0';

		String *result = new String(chars);
		delete[] chars;

		return result->Autorelease();
	}

	const String *OculusWindow::GetPreferredAudioOutputDeviceID() const
	{
		WCHAR idString[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
		if(OVR_SUCCESS(ovr_GetAudioDeviceOutGuidStr(idString)))
		{
			return RNSTR(StringForLPWSTR(idString));
		}

		return nullptr;
	}

	const String *OculusWindow::GetPreferredAudioInputDeviceID() const
	{
		WCHAR idString[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
		if(OVR_SUCCESS(ovr_GetAudioDeviceInGuidStr(idString)))
		{
			return RNSTR(StringForLPWSTR(idString));
		}

		return nullptr;
	}

	RenderingDevice *OculusWindow::GetOutputDevice() const
	{
		return nullptr;
	}
}

