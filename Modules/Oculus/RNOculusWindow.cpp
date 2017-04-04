//
//  RNOculusWindow.cpp
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusSwapChain.h"
#include "RNOculusWindow.h"

namespace RN
{
	RNDefineMeta(OculusWindow, Window)

	OculusWindow::OculusWindow()
	{
		_swapChain = new OculusSwapChain();
	}

	OculusWindow::~OculusWindow()
	{
		_swapChain->Release();
	}

	Vector2 OculusWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *OculusWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
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

	void OculusWindow::UpdateTrackingData(float near, float far)
	{
		_swapChain->UpdatePredictedPose();

		_hmdTrackingState.eyeOffset[0] = GetVectorForOVRVector(_swapChain->_hmdToEyeViewOffset[0]);
		_hmdTrackingState.eyeOffset[1] = GetVectorForOVRVector(_swapChain->_hmdToEyeViewOffset[1]);
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(ovrMatrix4f_Projection(_swapChain->_layer.Fov[0], near, far, ovrProjection_None));
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(ovrMatrix4f_Projection(_swapChain->_layer.Fov[1], near, far, ovrProjection_None));
		_hmdTrackingState.position = GetVectorForOVRVector(_swapChain->_hmdState.HeadPose.ThePose.Position);
		_hmdTrackingState.rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HeadPose.ThePose.Orientation);

		ovrInputState inputState;
		if(OVR_SUCCESS(ovr_GetInputState(_swapChain->_session, ovrControllerType_Touch, &inputState)))
		{
			_touchTrackingState[0].active = (inputState.ControllerType & ovrControllerType_LTouch);
			_touchTrackingState[0].position = GetVectorForOVRVector(_swapChain->_hmdState.HandPoses[0].ThePose.Position);
			_touchTrackingState[0].rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HandPoses[0].ThePose.Orientation);
			_touchTrackingState[0].thumbstick = GetVectorForOVRVector(inputState.Thumbstick[0]);
			_touchTrackingState[0].indexTrigger = inputState.IndexTrigger[0];
			_touchTrackingState[0].handTrigger = inputState.HandTrigger[0];

			_touchTrackingState[1].active = (inputState.ControllerType & ovrControllerType_RTouch);
			_touchTrackingState[1].position = GetVectorForOVRVector(_swapChain->_hmdState.HandPoses[1].ThePose.Position);
			_touchTrackingState[1].rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HandPoses[1].ThePose.Orientation);
			_touchTrackingState[1].thumbstick = GetVectorForOVRVector(inputState.Thumbstick[1]);
			_touchTrackingState[1].indexTrigger = inputState.IndexTrigger[1];
			_touchTrackingState[1].handTrigger = inputState.HandTrigger[1];

			_touchTrackingState[0].button[OculusTouchTrackingState::Button::AX] = inputState.Buttons & ovrButton_X;
			_touchTrackingState[0].button[OculusTouchTrackingState::Button::BY] = inputState.Buttons & ovrButton_Y;
			_touchTrackingState[0].button[OculusTouchTrackingState::Button::Thumb] = inputState.Buttons & ovrButton_LThumb;
			_touchTrackingState[0].button[OculusTouchTrackingState::Button::Enter] = inputState.Buttons & ovrButton_Enter;

			_touchTrackingState[1].button[OculusTouchTrackingState::Button::AX] = inputState.Buttons & ovrButton_A;
			_touchTrackingState[1].button[OculusTouchTrackingState::Button::BY] = inputState.Buttons & ovrButton_B;
			_touchTrackingState[1].button[OculusTouchTrackingState::Button::Thumb] = inputState.Buttons & ovrButton_RThumb;
			_touchTrackingState[1].button[OculusTouchTrackingState::Button::Enter] = false;
		}
		else
		{
			_touchTrackingState[0].active = false;
			_touchTrackingState[1].active = false;
		}
	}

	const OculusHMDTrackingState &OculusWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const OculusTouchTrackingState &OculusWindow::GetTouchTrackingState(int hand) const
	{
		return _touchTrackingState[hand];
	}

	void OculusWindow::SubmitTouchHaptics(int hand, const OculusTouchHaptics &haptics)
	{
		ovrHapticsBuffer buffer;
		buffer.SubmitMode = ovrHapticsBufferSubmit_Enqueue;
		buffer.SamplesCount = haptics.sampleCount;
		buffer.Samples = haptics.samples;
		ovr_SubmitControllerVibration(_swapChain->_session, hand?ovrControllerType_RTouch:ovrControllerType_LTouch, &buffer);
	}
}
