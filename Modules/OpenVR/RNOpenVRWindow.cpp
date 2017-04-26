//
//  RNOpenVRWindow.cpp
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenVRSwapChain.h"
#include "RNOpenVRWindow.h"

namespace RN
{
	RNDefineMeta(OpenVRWindow, VRWindow)

	OpenVRWindow::OpenVRWindow()
	{
		_swapChain = new OpenVRSwapChain();
	}

	OpenVRWindow::~OpenVRWindow()
	{
		_swapChain->Release();
	}

	Vector2 OpenVRWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *OpenVRWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	static Matrix GetMatrixForOVRMatrix(const vr::HmdMatrix44_t &ovrMatrix)
	{
		Matrix result;
		for(int q = 0; q < 4; q++)
		{
			for(int t = 0; t < 4; t++)
			{
				result.m[q * 4 + t] = ovrMatrix.m[t][q];
			}
		}
		return result;
	}

	static Matrix GetRotationMatrixForOVRMatrix(const vr::HmdMatrix34_t &ovrMatrix)
	{
		Matrix result;
		for(int q = 0; q < 3; q++)
		{
			for(int t = 0; t < 3; t++)
			{
				result.m[q * 4 + t] = ovrMatrix.m[t][q];
			}
		}
		return result;
	}

	void OpenVRWindow::UpdateTrackingData(float near, float far)
	{
		_swapChain->UpdatePredictedPose();

		vr::HmdMatrix44_t leftProjection = _swapChain->_hmd->GetProjectionMatrix(vr::Eye_Left, near, far);
		vr::HmdMatrix44_t rightProjection = _swapChain->_hmd->GetProjectionMatrix(vr::Eye_Right, near, far);

		_hmdTrackingState.eyeOffset[0] = _swapChain->_hmdToEyeViewOffset[0];
		_hmdTrackingState.eyeOffset[1] = _swapChain->_hmdToEyeViewOffset[1];
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(leftProjection);
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(rightProjection);

		if (_swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			vr::HmdMatrix34_t headPose = _swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
			Matrix rotationPose = GetRotationMatrixForOVRMatrix(headPose);
			_hmdTrackingState.position.x = headPose.m[0][3];
			_hmdTrackingState.position.y = headPose.m[1][3];
			_hmdTrackingState.position.z = headPose.m[2][3];
			_hmdTrackingState.rotation = rotationPose.GetEulerAngle();
		}


		for(int nDevice = 1; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
		{
			uint8 handIndex = -1;
			switch(_swapChain->_hmd->GetTrackedDeviceClass(nDevice))
			{
			case vr::TrackedDeviceClass_Controller:
			{
				vr::ETrackedControllerRole role = _swapChain->_hmd->GetControllerRoleForTrackedDeviceIndex(nDevice);
				switch(role)
				{
				case vr::TrackedControllerRole_LeftHand:
					handIndex = 0;
					break;

				case vr::TrackedControllerRole_RightHand:
					handIndex = 1;
					break;

				default:
					break;
				}
					
				break;
			}
					
			default:
				break;
			}

			if(handIndex != -1)
			{
				if(_swapChain->_frameDevicePose[nDevice].bPoseIsValid)
				{
					_controllerTrackingState[handIndex].active = true;

					vr::HmdMatrix34_t handPose = _swapChain->_frameDevicePose[nDevice].mDeviceToAbsoluteTracking;
					Matrix rotationPose = GetRotationMatrixForOVRMatrix(handPose);
					_controllerTrackingState[handIndex].position.x = handPose.m[0][3];
					_controllerTrackingState[handIndex].position.y = handPose.m[1][3];
					_controllerTrackingState[handIndex].position.z = handPose.m[2][3];
					_controllerTrackingState[handIndex].rotation = rotationPose.GetEulerAngle();

					vr::VRControllerState_t controllerState;
					_swapChain->_hmd->GetControllerState(nDevice, &controllerState, sizeof(controllerState));

					_controllerTrackingState[handIndex].thumbstick.x = controllerState.rAxis[0].x;
					_controllerTrackingState[handIndex].thumbstick.y = controllerState.rAxis[0].y;
					_controllerTrackingState[handIndex].indexTrigger = controllerState.rAxis[1].x;
					_controllerTrackingState[handIndex].handTrigger = controllerState.rAxis[2].x;

					//TODO: Implement remaining buttons
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::AX] = controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A);
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::BY] = false;// controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A + 1);
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Thumb] = false;// inputState.Buttons & ovrButton_LThumb;
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Enter] = false;// inputState.Buttons & ovrButton_Enter;
				}
				else
				{
					_controllerTrackingState[handIndex].active = false;
				}
			}
		}
	}

	const VRHMDTrackingState &OpenVRWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const VRControllerTrackingState &OpenVRWindow::GetControllerTrackingState(int hand) const
	{
		return _controllerTrackingState[hand];
	}

	void OpenVRWindow::SubmitControllerHaptics(int hand, const VRControllerHaptics &haptics)
	{
/*		ovrHapticsBuffer buffer;
		buffer.SubmitMode = ovrHapticsBufferSubmit_Enqueue;
		buffer.SamplesCount = haptics.sampleCount;
		buffer.Samples = haptics.samples;
		ovr_SubmitControllerVibration(_swapChain->_session, hand?ovrControllerType_RTouch:ovrControllerType_LTouch, &buffer);*/
	}
}
