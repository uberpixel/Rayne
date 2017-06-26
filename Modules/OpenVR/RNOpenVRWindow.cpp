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

		OpenVRWindow::OpenVRWindow(const SwapChainDescriptor &descriptor) : _currentHapticsIndex{ 500, 500 }, _remainingHapticsDelta(0.0f), _lastSizeChangeTimer(0.0f)
	{
		_swapChain = new OpenVRSwapChain(descriptor);
		_hmdTrackingState.position = Vector3(0.0f, 1.0f, 0.0f);
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

	uint32 OpenVRWindow::GetEyePadding() const
	{
		return OpenVRSwapChain::kEyePadding;
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

	void OpenVRWindow::Update(float delta, float near, float far)
	{
		uint32 recommendedWidth;
		uint32 recommendedHeight;
		_swapChain->_hmd->GetRecommendedRenderTargetSize(&recommendedWidth, &recommendedHeight);
		Vector2 newSize(recommendedWidth * 2 + OpenVRSwapChain::kEyePadding, recommendedHeight);

		if(newSize.GetSquaredDistance(_lastSize) > 0.001f)
		{
			_lastSizeChangeTimer = 0.0f;
		}
		_lastSize = newSize;

		if(_lastSizeChangeTimer > 0.1f && newSize.GetSquaredDistance(_swapChain->_size) > 0.001f)
		{
			UpdateSize(newSize);
		}

		_lastSizeChangeTimer += delta;

		uint16 handDevices[2] = { vr::k_unMaxTrackedDeviceCount, vr::k_unMaxTrackedDeviceCount };
		_swapChain->UpdatePredictedPose();

		vr::HmdMatrix44_t leftProjection = _swapChain->_hmd->GetProjectionMatrix(vr::Eye_Left, near, far);
		vr::HmdMatrix44_t rightProjection = _swapChain->_hmd->GetProjectionMatrix(vr::Eye_Right, near, far);

		_hmdTrackingState.eyeOffset[0] = _swapChain->_hmdToEyeViewOffset[0];
		_hmdTrackingState.eyeOffset[1] = _swapChain->_hmdToEyeViewOffset[1];
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(leftProjection);
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(rightProjection);

		if(_swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
		{
			vr::HmdMatrix34_t headPose = _swapChain->_frameDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
			Matrix rotationPose = GetRotationMatrixForOVRMatrix(headPose);
			_hmdTrackingState.position.x = headPose.m[0][3];
			_hmdTrackingState.position.y = headPose.m[1][3];
			_hmdTrackingState.position.z = headPose.m[2][3];
			_hmdTrackingState.rotation = rotationPose.GetEulerAngle();
		}

		_hmdTrackingState.mode = vr::VROverlay()->IsDashboardVisible() ? VRHMDTrackingState::Mode::Paused : VRHMDTrackingState::Mode::Rendering;
		vr::VREvent_t event;
		while(_swapChain->_hmd->PollNextEvent(&event, sizeof(event)))
		{
			//TODO: Handle more OpenVR events
			switch(event.eventType)
			{
			case vr::VREvent_Quit:
				_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
				break;
			}
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

			if(handIndex < 2)
			{
				handDevices[handIndex] = nDevice;
				if(_swapChain->_frameDevicePose[nDevice].bPoseIsValid)
				{
					_controllerTrackingState[handIndex].active = true;

					vr::HmdMatrix34_t handPose = _swapChain->_frameDevicePose[nDevice].mDeviceToAbsoluteTracking;
					Matrix rotationPose = GetRotationMatrixForOVRMatrix(handPose);
					_controllerTrackingState[handIndex].rotation = rotationPose.GetEulerAngle();
					_controllerTrackingState[handIndex].position.x = handPose.m[0][3];
					_controllerTrackingState[handIndex].position.y = handPose.m[1][3];
					_controllerTrackingState[handIndex].position.z = handPose.m[2][3];
					_controllerTrackingState[handIndex].position += _controllerTrackingState[handIndex].rotation.GetRotatedVector(Vector3(0.0f, -0.01f, 0.05f));

					vr::VRControllerState_t controllerState;
					_swapChain->_hmd->GetControllerState(nDevice, &controllerState, sizeof(controllerState));

					_controllerTrackingState[handIndex].thumbstick.x = controllerState.rAxis[0].x;
					_controllerTrackingState[handIndex].thumbstick.y = controllerState.rAxis[0].y;
					_controllerTrackingState[handIndex].indexTrigger = controllerState.rAxis[1].x;
					_controllerTrackingState[handIndex].handTrigger = controllerState.rAxis[2].x;

					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::AX] = controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A);
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::BY] = controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);	//For touch this is the B/Y button
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Stick] = controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad);
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Start] = controllerState.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);	//But it also kinda correspondsto this one... not sure what to do...
				}
				else
				{
					_controllerTrackingState[handIndex].active = false;
				}
			}
		}

		//Update haptics
		_remainingHapticsDelta += delta;
		uint16 hapticsIncrement = roundf(_remainingHapticsDelta / 0.003125f);
		_remainingHapticsDelta -= static_cast<float>(hapticsIncrement)*0.003125f;

		for(uint8 hand = 0; hand < 2; hand++)
		{
			uint16 device = handDevices[hand];
			if(_currentHapticsIndex[hand] < _haptics[hand].sampleCount && device < vr::k_unMaxTrackedDeviceCount)
			{
				if(_currentHapticsIndex[hand] < _haptics[hand].sampleCount)
				{
					if(_haptics[hand].samples[_currentHapticsIndex[hand]] > 0)
					{
						uint32 sample = _haptics[hand].samples[_currentHapticsIndex[hand]];
						sample *= 3999;
						sample /= 255;
						_swapChain->_hmd->TriggerHapticPulse(device, 0, sample);
					}
				}
				else
				{
					_currentHapticsIndex[hand] = 300;
				}

				_currentHapticsIndex[hand] += hapticsIncrement;
			}
		}
	}

	void OpenVRWindow::UpdateSize(const Vector2 &size)
	{
		//TODO: Enable again once SteamVR is fixed...
//		_swapChain->ResizeSwapchain(size);
//		NotificationManager::GetSharedInstance()->PostNotification(kRNWindowDidChangeSize, this);
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
		_currentHapticsIndex[hand] = 0;
		_haptics[hand] = haptics;
	}
}
