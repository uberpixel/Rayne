//
//  RNOculusMobileWindow.cpp
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusMobileVulkanSwapChain.h"
#include "RNOculusMobileWindow.h"

#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h> // for native window JNI
#include <android_native_app_glue.h>

#include "VrApi_Vulkan.h"
#include "VrApi_Helpers.h"
#include "VrApi_Input.h"

namespace RN
{
	RNDefineMeta(OculusMobileWindow, VRWindow)

	OculusMobileWindow::OculusMobileWindow() : _swapChain(nullptr)
	{
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

		ovrJava *java = new ovrJava;
		java->Vm = app->activity->vm;
		java->Vm->AttachCurrentThread(&java->Env, NULL);
		java->ActivityObject = app->activity->clazz;

		_java = static_cast<ovrJava*>(java);

		// Note that AttachCurrentThread will reset the thread name.
		prctl(PR_SET_NAME, (long)"Rayne::Main", 0, 0, 0);

		ovrInitParms initParams = vrapi_DefaultInitParms(java);
		initParams.GraphicsAPI = VRAPI_GRAPHICS_API_VULKAN_1;
		int32_t initResult = vrapi_Initialize(&initParams);
		if(initResult != VRAPI_INITIALIZE_SUCCESS)
		{
			return;
		}
	}

	OculusMobileWindow::~OculusMobileWindow()
	{
		StopRendering();

		vrapi_Shutdown();

		ovrJava *java = static_cast<ovrJava*>(_java);
        java->Vm->DetachCurrentThread();
        delete java;
	}

	void OculusMobileWindow::StartRendering(const SwapChainDescriptor &descriptor)
	{
		ovrJava *java = static_cast<ovrJava*>(_java);
		_swapChain = new OculusMobileVulkanSwapChain(descriptor, *java);
	}

	void OculusMobileWindow::StopRendering()
	{
		SafeRelease(_swapChain);
	}

	bool OculusMobileWindow::IsRendering() const
	{
		return true;
	}

	Vector2 OculusMobileWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *OculusMobileWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	uint32 OculusMobileWindow::GetEyePadding() const
	{
		return OculusMobileVulkanSwapChain::kEyePadding;
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

	void OculusMobileWindow::Update(float delta, float near, float far)
	{
		if(!_swapChain->_session) return;

		_swapChain->UpdatePredictedPose();

		float eyeDistance = vrapi_GetInterpupillaryDistance(&_swapChain->_hmdState);
		_hmdTrackingState.eyeOffset[0] = Vector3(-eyeDistance/2.0f, 0.0f, 0.0f);
		_hmdTrackingState.eyeOffset[1] = Vector3(eyeDistance/2.0f, 0.0f, 0.0f);
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(_swapChain->_hmdState.Eye[0].ProjectionMatrix);//ovrMatrix4f_Projection(_swapChain->_imageLayer.Fov[0], near, far, ovrProjection_None));
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(_swapChain->_hmdState.Eye[1].ProjectionMatrix);//ovrMatrix4f_Projection(_swapChain->_imageLayer.Fov[1], near, far, ovrProjection_None));

		_hmdTrackingState.position = GetVectorForOVRVector(_swapChain->_hmdState.HeadPose.Pose.Position);
		_hmdTrackingState.rotation = GetQuaternionForOVRQuaternion(_swapChain->_hmdState.HeadPose.Pose.Orientation);

/*		_swapChain->SetProjection(_hmdTrackingState.eyeProjection[0].m[10], _hmdTrackingState.eyeProjection[0].m[14], _hmdTrackingState.eyeProjection[0].m[11]);

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
			ovrSessionStatus status;
			ovr_GetSessionStatus(_swapChain->_session, &status);
			if(status.HasInputFocus)
			{
				_hmdTrackingState.mode = VRHMDTrackingState::Mode::Rendering;
			}
			else
			{
				_hmdTrackingState.mode = VRHMDTrackingState::Mode::Paused;
			}
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
		}*/

		_controllerTrackingState[0].active = false;
		_controllerTrackingState[0].tracking = false;
		_controllerTrackingState[1].active = false;
		_controllerTrackingState[1].tracking = false;

		ovrInputCapabilityHeader capsHeader;
		if(vrapi_EnumerateInputDevices(_swapChain->_session, 0, &capsHeader) >= 0)
		{
			if(capsHeader.Type == ovrControllerType_TrackedRemote)
			{
				ovrInputTrackedRemoteCapabilities remoteCaps;
				remoteCaps.Header = capsHeader;
				if(vrapi_GetInputDeviceCapabilities(_swapChain->_session, &remoteCaps.Header) >= 0)
				{
					int handIndex = (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand)?1:0;

					_controllerTrackingState[handIndex].active = true;
					_controllerTrackingState[handIndex].tracking = true;
					_controllerTrackingState[handIndex].controllerID = 0;

					ovrInputStateTrackedRemote remoteState;
					remoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
					if(vrapi_GetCurrentInputState(_swapChain->_session, remoteCaps.Header.DeviceID, &remoteState.Header) >= 0)
					{
						_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::AX] = false;
						_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::BY] = false;
						_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Pad] = remoteState.Buttons & ovrButton_Enter;
						_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Start] = remoteState.Buttons & ovrButton_Back;

						Vector2 trackpadPosition;
						if(remoteState.TrackpadStatus > 0)
						{
							Vector2 trackpadMax(remoteCaps.TrackpadMaxX, remoteCaps.TrackpadMaxY);
							trackpadPosition = (GetVectorForOVRVector(remoteState.TrackpadPosition) / trackpadMax) * 2.0f - 1.0f;
						}
						_controllerTrackingState[handIndex].trackpad = trackpadPosition;
						_controllerTrackingState[handIndex].indexTrigger = (remoteState.Buttons & ovrButton_A)? 1.0f:0.0f;
						_controllerTrackingState[handIndex].handTrigger = 0.0f;
					}

					ovrTracking trackingState;
					if(vrapi_GetInputTrackingState(_swapChain->_session, remoteCaps.Header.DeviceID, _swapChain->_predictedDisplayTime, &trackingState) >= 0)
					{
						_controllerTrackingState[handIndex].position = GetVectorForOVRVector(trackingState.HeadPose.Pose.Position);
						_controllerTrackingState[handIndex].rotation = GetQuaternionForOVRQuaternion(trackingState.HeadPose.Pose.Orientation);
						//_controllerTrackingState[0].rotation *= RN::Vector3(0.0f, 45.0f, 0.0f);
					}
				}
			}
		}
	}

	const VRHMDTrackingState &OculusMobileWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const VRControllerTrackingState &OculusMobileWindow::GetControllerTrackingState(uint8 index) const
	{
		return _controllerTrackingState[index];
	}

	const VRControllerTrackingState &OculusMobileWindow::GetTrackerTrackingState(uint8 index) const
	{
		return _trackerTrackingState;
	}

	void OculusMobileWindow::SubmitControllerHaptics(uint8 controllerID, const VRControllerHaptics &haptics)
	{
/*		ovrHapticsBuffer buffer;
		buffer.SubmitMode = ovrHapticsBufferSubmit_Enqueue;
		buffer.SamplesCount = haptics.sampleCount;
		buffer.Samples = haptics.samples;
		ovr_SubmitControllerVibration(_swapChain->_session, controllerID?ovrControllerType_RTouch:ovrControllerType_LTouch, &buffer);*/
	}

	const String *OculusMobileWindow::GetPreferredAudioOutputDeviceID() const
	{
		return nullptr;
	}

	const String *OculusMobileWindow::GetPreferredAudioInputDeviceID() const
	{
		return nullptr;
	}

	RenderingDevice *OculusMobileWindow::GetOutputDevice(RendererDescriptor *descriptor) const
	{
		return nullptr;
	}

	const Window::SwapChainDescriptor &OculusMobileWindow::GetSwapChainDescriptor() const
	{
		return _swapChain->GetSwapChainDescriptor();
	}

	Array *OculusMobileWindow::GetRequiredVulkanInstanceExtensions() const
	{
		char names[4096];
		uint32_t size = sizeof(names);
        vrapi_GetInstanceExtensionsVulkan(names, &size);

        String *extensionString = RNSTR(names);
        return extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
	}

    Array *OculusMobileWindow::GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *device) const
    {
    	char names[4096];
		uint32_t size = sizeof(names);
		vrapi_GetDeviceExtensionsVulkan(names, &size);

		String *extensionString = RNSTR(names);
		return extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
    }
}
