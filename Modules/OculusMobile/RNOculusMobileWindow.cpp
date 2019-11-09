//
//  RNOculusMobileWindow.cpp
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusMobileVulkanSwapChain.h"
#include "RNOculusMobileWindow.h"

#include <unistd.h>
#include <android/log.h>

#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h> // for native window JNI
#include <android_native_app_glue.h>

#include "VrApi.h"
#include "VrApi_Vulkan.h"
#include "VrApi_Helpers.h"
#include "VrApi_Input.h"
#include "VrApi_SystemUtils.h"

namespace RN
{
	RNDefineMeta(OculusMobileWindow, VRWindow)

	OculusMobileWindow::OculusMobileWindow() : _nativeWindow(nullptr), _session(nullptr), _actualFrameIndex(0), _predictedDisplayTime(0.0)
	{
		_swapChain[0] = nullptr;
		_swapChain[1] = nullptr;

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

		_mainThreadID = gettid();

		_hmdTrackingState.type = vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_HEADSET_TYPE) < VRAPI_HEADSET_TYPE_OCULUSQUEST? VRHMDTrackingState::Type::ThreeDegreesOfFreedom : VRHMDTrackingState::Type::SixDegreesOfFreedom;

		RNInfo(GetHMDInfoDescription());
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

		VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

		ovrSystemCreateInfoVulkan systemInfo;
		systemInfo.Instance = renderer->GetVulkanInstance()->GetInstance();
		systemInfo.PhysicalDevice = renderer->GetVulkanDevice()->GetPhysicalDevice();
		systemInfo.Device = renderer->GetVulkanDevice()->GetDevice();
		ovrResult result = vrapi_CreateSystemVulkan(&systemInfo);
		if(result != VRAPI_INITIALIZE_SUCCESS)
		{
			return;
		}

		//1:1 mapping for center according to docs would be 1536x1536, returned is 1024*1024 for GO, higher on quest
		Vector2 eyeRenderSize;
        eyeRenderSize.x = vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
        eyeRenderSize.y = vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);

		_swapChain[0] = new OculusMobileVulkanSwapChain(descriptor, eyeRenderSize);
		_swapChain[1] = new OculusMobileVulkanSwapChain(descriptor, eyeRenderSize);

		_swapChain[1]->_presentEvent = [this](){
			if(_session)
			{
				ovrLayerProjection2 gameLayer = vrapi_DefaultLayerProjection2();

				gameLayer.HeadPose = _swapChain[0]->_hmdState.HeadPose;
				for(int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++)
				{
					gameLayer.Textures[eye].ColorSwapChain = _swapChain[eye]->_colorSwapChain;
					gameLayer.Textures[eye].SwapChainIndex = _swapChain[eye]->_semaphoreIndex;
					gameLayer.Textures[eye].TexCoordsFromTanAngles = _swapChain[eye]->GetTanAngleMatrixForEye(eye);
					gameLayer.Textures[eye].TextureRect.x = 0.0f;
					gameLayer.Textures[eye].TextureRect.y = 0.0f;
					gameLayer.Textures[eye].TextureRect.width = 1.0f;
					gameLayer.Textures[eye].TextureRect.height = 1.0f;
				}
				gameLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

				const ovrLayerHeader2 * layers[] = { &gameLayer.Header };

				ovrSubmitFrameDescription2 frameDescription = {};
				frameDescription.Flags = 0;
				frameDescription.SwapInterval = 1;
				frameDescription.FrameIndex = _actualFrameIndex;
				frameDescription.DisplayTime = _predictedDisplayTime;
				frameDescription.LayerCount = 1;
				frameDescription.Layers = layers;

				vrapi_SubmitFrame2(static_cast<ovrMobile*>(_session), &frameDescription);
			}
		};

		NotificationManager::GetSharedInstance()->AddSubscriber(kRNAndroidWindowDidChange, [this](Notification *notification) {
				if(notification->GetName()->IsEqual(kRNAndroidWindowDidChange))
				{
					UpdateVRMode();
				}
			}, this);

		UpdateVRMode();
	}

	void OculusMobileWindow::StopRendering()
	{
		NotificationManager::GetSharedInstance()->RemoveSubscriber(kRNAndroidWindowDidChange, this);

		if(_session)
		{
			vrapi_LeaveVrMode(static_cast<ovrMobile*>(_session));
		}

		SafeRelease(_swapChain[0]);
		SafeRelease(_swapChain[1]);

		vrapi_DestroySystemVulkan();
	}

	bool OculusMobileWindow::IsRendering() const
	{
		return true;
	}

	Vector2 OculusMobileWindow::GetSize() const
	{
		return _swapChain[0]->GetSize();
	}

	Framebuffer *OculusMobileWindow::GetFramebuffer() const
	{
		return _swapChain[0]->GetFramebuffer();
	}

	Framebuffer *OculusMobileWindow::GetFramebuffer(uint8 eye) const
	{
		RN_ASSERT(eye < 2, "Eye Index need to be 0 or 1");
		return _swapChain[eye]->GetFramebuffer();
	}

	uint32 OculusMobileWindow::GetEyePadding() const
	{
		return 0;
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
		_hmdTrackingState.mode = VRHMDTrackingState::Mode::Paused;

		if(!_session) return;

		vrapi_SetTrackingSpace(static_cast<ovrMobile*>(_session), VRAPI_TRACKING_SPACE_LOCAL_FLOOR);

		_actualFrameIndex++;
        _predictedDisplayTime = vrapi_GetPredictedDisplayTime(static_cast<ovrMobile*>(_session), _actualFrameIndex);

		ovrTracking2 hmdState = vrapi_GetPredictedTracking2(static_cast<ovrMobile*>(_session), _predictedDisplayTime);
		_swapChain[0]->_hmdState = hmdState;
		_swapChain[1]->_hmdState = hmdState;

		float eyeDistance = vrapi_GetInterpupillaryDistance(&hmdState);
		_hmdTrackingState.eyeOffset[0] = Vector3(-eyeDistance/2.0f, 0.0f, 0.0f);
		_hmdTrackingState.eyeOffset[1] = Vector3(eyeDistance/2.0f, 0.0f, 0.0f);
		_hmdTrackingState.eyeProjection[0] = GetMatrixForOVRMatrix(hmdState.Eye[0].ProjectionMatrix);
		_hmdTrackingState.eyeProjection[1] = GetMatrixForOVRMatrix(hmdState.Eye[1].ProjectionMatrix);

		_hmdTrackingState.position = GetVectorForOVRVector(hmdState.HeadPose.Pose.Position);
		_hmdTrackingState.rotation = GetQuaternionForOVRQuaternion(hmdState.HeadPose.Pose.Orientation);

		_hmdTrackingState.mode = VRHMDTrackingState::Mode::Rendering;

/*
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
			ovr_GetSessionStatus(_session, &status);
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
		ovr_GetSessionStatus(_session, &sessionStatus);

		if(sessionStatus.ShouldQuit)
		{
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Disconnected;
		}
		if(sessionStatus.ShouldRecenter)
		{
			ovr_RecenterTrackingOrigin(_session); // or ovr_ClearShouldRecenterFlag(_session) to ignore the request.
		}*/

		_controllerTrackingState[0].type = static_cast<VRControllerTrackingState::Type>(_hmdTrackingState.type);
		_controllerTrackingState[0].hasHaptics = false;
		_controllerTrackingState[0].active = false;
		_controllerTrackingState[0].tracking = false;
		_controllerTrackingState[0].hapticsSampleLength = 0.0;
		_controllerTrackingState[0].hapticsMaxSamples = 0;
		_controllerTrackingState[1].type = static_cast<VRControllerTrackingState::Type>(_hmdTrackingState.type);
		_controllerTrackingState[1].active = false;
		_controllerTrackingState[1].tracking = false;
		_controllerTrackingState[1].hasHaptics = false;
		_controllerTrackingState[1].hapticsSampleLength = 0.0;
		_controllerTrackingState[1].hapticsMaxSamples = 0;

		ovrInputCapabilityHeader capsHeader;
		int i = 0;
		while(vrapi_EnumerateInputDevices(static_cast<ovrMobile*>(_session), i, &capsHeader) >= 0 && i < 2)
		{
			i += 1;
			if(capsHeader.Type == ovrControllerType_TrackedRemote)
			{
				ovrInputTrackedRemoteCapabilities remoteCaps;
				remoteCaps.Header = capsHeader;
				if(vrapi_GetInputDeviceCapabilities(static_cast<ovrMobile*>(_session), &remoteCaps.Header) >= 0)
				{
					int handIndex = (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand)?1:0;

					_controllerTrackingState[handIndex].hasHaptics = (remoteCaps.ControllerCapabilities & ovrControllerCaps_HasBufferedHapticVibration);
					_controllerTrackingState[handIndex].hapticsSampleLength = static_cast<double>(remoteCaps.HapticSampleDurationMS)/1000.0;
					_controllerTrackingState[handIndex].hapticsMaxSamples = remoteCaps.HapticSamplesMax;

					_controllerTrackingState[handIndex].active = true;
					_controllerTrackingState[handIndex].tracking = true;
					_controllerTrackingState[handIndex].controllerID = remoteCaps.Header.DeviceID;

					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::AX] = false;
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::BY] = false;
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Stick] = false;
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Pad] = false;
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Start] = false;
					_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::PadTouched] = false;

					//_controllerTrackingState[handIndex].trackpad = Vector2();
					_controllerTrackingState[handIndex].indexTrigger = 0.0f;
					_controllerTrackingState[handIndex].handTrigger = 0.0f;
					_controllerTrackingState[handIndex].thumbstick = Vector2();

					ovrInputStateTrackedRemote remoteState;
					remoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
					if(vrapi_GetCurrentInputState(static_cast<ovrMobile*>(_session), remoteCaps.Header.DeviceID, &remoteState.Header) >= 0)
					{
						if((remoteCaps.ControllerCapabilities & ovrControllerCaps_HasTrackpad))
						{
							_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Pad] = remoteState.Buttons & ovrButton_Enter;

							if(remoteState.TrackpadStatus > 0 || remoteState.Buttons & ovrButton_Enter)
							{
								_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::PadTouched] = true;
							}

							Vector2 trackpadMax(remoteCaps.TrackpadMaxX, remoteCaps.TrackpadMaxY);
							Vector2 trackpadPosition = (GetVectorForOVRVector(remoteState.TrackpadPosition) / trackpadMax) * 2.0f - 1.0f;
							_controllerTrackingState[handIndex].trackpad = trackpadPosition;
						}

						if((remoteCaps.ControllerCapabilities & ovrControllerCaps_ModelOculusGo) || (remoteCaps.ControllerCapabilities & ovrControllerCaps_ModelGearVR))
						{
							_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Start] = remoteState.Buttons & ovrButton_Back;
							_controllerTrackingState[handIndex].indexTrigger = (remoteState.Buttons & ovrButton_A) ? 1.0f : 0.0f;
						}
						else
						{
							if((remoteCaps.ControllerCapabilities & ovrControllerCaps_HasAnalogIndexTrigger))
							{
								_controllerTrackingState[handIndex].indexTrigger = remoteState.IndexTrigger;
							}
							_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Start] = (remoteState.Buttons & ovrButton_Enter);
							_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::AX] = (remoteState.Buttons & ovrButton_A) || (remoteState.Buttons & ovrButton_X);
							_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::BY] = (remoteState.Buttons & ovrButton_B) || (remoteState.Buttons & ovrButton_Y);
						}

						if((remoteCaps.ControllerCapabilities & ovrControllerCaps_HasJoystick))
						{
							_controllerTrackingState[handIndex].thumbstick = Vector2(remoteState.Joystick.x, remoteState.Joystick.y);
							_controllerTrackingState[handIndex].button[VRControllerTrackingState::Button::Stick] = (remoteState.Buttons & ovrButton_Joystick);
						}

						if((remoteCaps.ControllerCapabilities & ovrControllerCaps_HasAnalogGripTrigger))
						{
							_controllerTrackingState[handIndex].handTrigger = remoteState.GripTrigger;
						}
					}

					ovrTracking trackingState;
					if(vrapi_GetInputTrackingState(static_cast<ovrMobile*>(_session), remoteCaps.Header.DeviceID, _predictedDisplayTime, &trackingState) >= 0)
					{
						_controllerTrackingState[handIndex].position = GetVectorForOVRVector(trackingState.HeadPose.Pose.Position);
						_controllerTrackingState[handIndex].rotation = GetQuaternionForOVRQuaternion(trackingState.HeadPose.Pose.Orientation);
						_controllerTrackingState[handIndex].rotation *= RN::Vector3(0.0f, 45.0f, 0.0f);

						_controllerTrackingState[handIndex].velocityLinear = GetVectorForOVRVector(trackingState.HeadPose.LinearVelocity);
						_controllerTrackingState[handIndex].velocityAngular.x = trackingState.HeadPose.AngularVelocity.y;
						_controllerTrackingState[handIndex].velocityAngular.y = trackingState.HeadPose.AngularVelocity.x;
						_controllerTrackingState[handIndex].velocityAngular.z = trackingState.HeadPose.AngularVelocity.z;
					}
				}
			}
		}
	}

	void OculusMobileWindow::UpdateVRMode()
	{
		RNDebug(RNCSTR("UpdateVRMode called"));

		if(!_nativeWindow)
		{
			if(!_session)
			{
				android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
				_nativeWindow = app->window;

				VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
				ovrModeParmsVulkan params = vrapi_DefaultModeParmsVulkan(static_cast<ovrJava*>(_java), (unsigned long long)renderer->GetWorkQueue());
				params.ModeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW | VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB;
				params.ModeParms.WindowSurface = (size_t)_nativeWindow;
				_session = vrapi_EnterVrMode((ovrModeParms *)&params);

				// If entering VR mode failed then the ANativeWindow was not valid.
				if(!_session)
				{
					RNDebug(RNCSTR("Invalid ANativeWindow!"));
					_nativeWindow = nullptr;
				}

				// Set performance parameters once we have entered VR mode and have a valid ovrMobile.
				if(_session)
				{
					RNDebug(RNCSTR("UpdateVRMode new session"));

					ovrMobile *session = static_cast<ovrMobile*>(_session);
					vrapi_SetDisplayRefreshRate(session, 72.0f); //72
					vrapi_SetRemoteEmulation(session, false);
					vrapi_SetClockLevels(session, 5, 5); //TODO: Set to 0, 0 for automatic clock levels, current setting keeps optimizations more comparable
					vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_MAIN, _mainThreadID);
    				vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_RENDERER, _mainThreadID);

					vrapi_SetExtraLatencyMode(session, VRAPI_EXTRA_LATENCY_MODE_ON);

					int hasFoveation = 0;
					vrapi_GetPropertyInt(static_cast<ovrJava*>(_java), (ovrProperty)VRAPI_SYS_PROP_FOVEATION_AVAILABLE, &hasFoveation);
					if(hasFoveation == VRAPI_TRUE)
					{
						RNDebug(RNCSTR("Enable Foveated Rendering"));
						vrapi_SetPropertyInt(static_cast<ovrJava*>(_java), VRAPI_FOVEATION_LEVEL, 2);
					}
				}
			}
		}
		else
		{
			_nativeWindow = nullptr;
			if(_session)
			{
				vrapi_LeaveVrMode(static_cast<ovrMobile*>(_session));
				_session = nullptr;

				RNDebug(RNCSTR("UpdateVRMode session lost"));
			}
		}
	}

	const String *OculusMobileWindow::GetHMDInfoDescription() const
	{
		ovrJava *java = static_cast<ovrJava*>(_java);
		String *description = new String("Using HMD: ");

		switch(vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_HEADSET_TYPE))
		{
			case VRAPI_HEADSET_TYPE_R320:
			case VRAPI_HEADSET_TYPE_R321:
			case VRAPI_HEADSET_TYPE_R322:
			case VRAPI_HEADSET_TYPE_R323:
			case VRAPI_HEADSET_TYPE_R324:
			case VRAPI_HEADSET_TYPE_R325:
				description->Append(RNCSTR("GearVR"));
				break;

			case VRAPI_HEADSET_TYPE_OCULUSGO:
			case VRAPI_HEADSET_TYPE_MIVR_STANDALONE:
				description->Append(RNCSTR("Oculus GO"));
				break;

			case VRAPI_HEADSET_TYPE_OCULUSQUEST:
				description->Append(RNCSTR("Oculus Quest"));
				break;
		}

		return description->Autorelease();
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

	void OculusMobileWindow::SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics)
	{
		if(!_session) return;
		if(!_controllerTrackingState[index].hasHaptics) return;

		ovrHapticBuffer hapticBuffer;
		hapticBuffer.BufferTime = _predictedDisplayTime;
		hapticBuffer.NumSamples = haptics.sampleCount;
		hapticBuffer.Terminated = false;
		hapticBuffer.HapticBuffer = haptics.samples;
		vrapi_SetHapticVibrationBuffer(static_cast<ovrMobile*>(_session), _controllerTrackingState[index].controllerID, &hapticBuffer);
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
		return _swapChain[0]->GetSwapChainDescriptor();
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

