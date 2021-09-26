//
//  RNOpenXRWindow.cpp
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNOpenXRWindow.h"
#include "RNOpenXRInternals.h"

/*

#include <android/log.h>

#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/native_window_jni.h> // for native window JNI
*/

#if RN_PLATFORM_ANDROID
#include <unistd.h>
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android_native_app_glue.h>
#endif

namespace RN
{
	RNDefineMeta(OpenXRWindow, VRWindow)

	static VRControllerTrackingState::Type GetControllerTypeForInteractionProfile(XrInstance instance, XrPath interactionProfile)
	{
		XrPath khronosSimpleController;
		xrStringToPath(instance, "/interaction_profiles/khr/simple_controller", &khronosSimpleController);

		XrPath oculusTouchController;
		xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &oculusTouchController);

		XrPath htcViveController;
		xrStringToPath(instance, "/interaction_profiles/htc/vive_controller", &htcViveController);

		XrPath valveIndexController;
		xrStringToPath(instance, "/interaction_profiles/valve/index_controller", &valveIndexController);

		XrPath microsoftMixedRealityController;
		xrStringToPath(instance, "/interaction_profiles/microsoft/motion_controller", &microsoftMixedRealityController);
		
		if(interactionProfile == khronosSimpleController)
		{
			return VRControllerTrackingState::Type::KhronosSimpleController;
		}
		else if(interactionProfile == oculusTouchController)
		{
			return VRControllerTrackingState::Type::OculusTouchController;
		}
		else if(interactionProfile == htcViveController)
		{
			return VRControllerTrackingState::Type::HTCViveController;
		}
		else if(interactionProfile == valveIndexController)
		{
			return VRControllerTrackingState::Type::ValveIndexController;
		}
		else if(interactionProfile == microsoftMixedRealityController)
		{
			return VRControllerTrackingState::Type::MicrosoftMixedRealityController;
		}

		return VRControllerTrackingState::Type::None;
	}

	OpenXRWindow::OpenXRWindow() : _internals(new OpenXRWindowInternals()), _swapChain(nullptr), _actualFrameIndex(0), _predictedDisplayTime(0.0), _currentHapticsIndex{0, 0}, _hapticsStopped{true, true}, _preferredFrameRate(0.0f), _minCPULevel(XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT), _minGPULevel(XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT), _fixedFoveatedRenderingLevel(2), _fixedFoveatedRenderingDynamic(false), _isSessionRunning(false), _hasSynchronization(false), _hasVisibility(false), _hasInputFocus(false)
	{
		_supportsVulkan = false;
		_supportsPreferredFramerate = false;
		_supportsPerformanceLevels = false;
		_supportsAndroidThreadType = false;
		_supportsFoveatedRendering = false;

		_internals->session = XR_NULL_HANDLE;

#if XR_USE_GRAPHICS_API_VULKAN
		_internals->GetVulkanInstanceExtensionsKHR = nullptr;
		_internals->GetVulkanDeviceExtensionsKHR = nullptr;
		_internals->GetVulkanGraphicsDeviceKHR = nullptr;
		_internals->GetVulkanGraphicsRequirementsKHR = nullptr;
#endif

		_internals->EnumerateDisplayRefreshRatesFB = nullptr;
		_internals->GetDisplayRefreshRateFB = nullptr;
		_internals->RequestDisplayRefreshRateFB = nullptr;

		_internals->PerfSettingsSetPerformanceLevelEXT = nullptr;

		_internals->CreateFoveationProfileFB = nullptr;
		_internals->DestroyFoveationProfileFB = nullptr;

		_internals->UpdateSwapchainFB = nullptr;
		_internals->GetSwapchainStateFB = nullptr;

#if XR_USE_PLATFORM_ANDROID
		_internals->SetAndroidApplicationThreadKHR = nullptr;
#endif

		
		std::vector<const char*> extensions;
		XrBaseInStructure *platformSpecificInstanceCreateInfo = nullptr;

#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

		PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
		if(XR_SUCCEEDED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)(&initializeLoader))))
		{
			XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid;
			memset(&loaderInitInfoAndroid, 0, sizeof(loaderInitInfoAndroid));
			loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
			loaderInitInfoAndroid.next = nullptr;
			loaderInitInfoAndroid.applicationVM = app->activity->vm;
			loaderInitInfoAndroid.applicationContext = app->activity->clazz;
			initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid);
		}

		extensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);

		XrInstanceCreateInfoAndroidKHR instanceCreateInfo = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
		instanceCreateInfo.applicationVM = app->activity->vm;
		instanceCreateInfo.applicationActivity = app->activity->clazz;

		platformSpecificInstanceCreateInfo = reinterpret_cast<XrBaseInStructure*>(&instanceCreateInfo);
		_mainThreadID = gettid();
#endif

#ifdef XR_USE_GRAPHICS_API_D3D12
		extensions.push_back(XR_KHR_D3D12_ENABLE_EXTENSION_NAME);
		_supportsD3D12 = true; //TODO: Only set to true if actually available!?
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
		extensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
		_supportsVulkan = true; //TODO: Only set to true if actually available!?
#endif

		uint32_t instanceExtensionCount;
		xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr);
		std::vector<XrExtensionProperties> allExtensions(instanceExtensionCount);
		for(XrExtensionProperties& extension : allExtensions)
		{
			extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		}
		xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)allExtensions.size(), &instanceExtensionCount, allExtensions.data());

		int numberOfSupportedFoveationExtensions = 0;
		RNDebug("Available Extensions (" << instanceExtensionCount << "):");
		for(const XrExtensionProperties& extension : allExtensions)
		{
			if(std::strcmp(extension.extensionName, XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsPreferredFramerate = true;
			}
			else if(std::strcmp(extension.extensionName, XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsPerformanceLevels = true;
			}
#if XR_USE_PLATFORM_ANDROID
			else if(std::strcmp(extension.extensionName, XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsAndroidThreadType = true;
			}
#endif

			else if(std::strcmp(extension.extensionName, XR_FB_FOVEATION_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				numberOfSupportedFoveationExtensions += 1;
			}
			else if(std::strcmp(extension.extensionName, XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				numberOfSupportedFoveationExtensions += 1;
			}
#if XR_USE_GRAPHICS_API_VULKAN
			else if(std::strcmp(extension.extensionName, XR_FB_FOVEATION_VULKAN_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				numberOfSupportedFoveationExtensions += 1;
			}
#endif
			//Needed to apply foveation profiles to the swapchain
			else if(std::strcmp(extension.extensionName, XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				numberOfSupportedFoveationExtensions += 1;
			}

			RNDebug("  Name: " << extension.extensionName << ", Spec Version: " << extension.extensionVersion);
		}

	  if(numberOfSupportedFoveationExtensions == 4)
		{
			_supportsFoveatedRendering = true;
		}

		XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
		createInfo.next = platformSpecificInstanceCreateInfo;
		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.enabledExtensionNames = extensions.data();

		strcpy(createInfo.applicationInfo.applicationName, "HelloXR");
		createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

		_internals->instance = XR_NULL_HANDLE;
	  if(xrCreateInstance(&createInfo, &_internals->instance) != XR_SUCCESS)
		{
			RN_ASSERT(false, "Failed creating OpenXR instance");
		}

		XrSystemGetInfo systemInfo;
		systemInfo.type = XR_TYPE_SYSTEM_GET_INFO;
		systemInfo.next = nullptr;
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		if(xrGetSystem(_internals->instance, &systemInfo, &_internals->systemID) != XR_SUCCESS)
		{
			RN_ASSERT(false, "No HMD found");
		}

		_internals->systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
		if(xrGetSystemProperties(_internals->instance, _internals->systemID, &_internals->systemProperties) != XR_SUCCESS)
		{
			RN_ASSERT(false, "Failed fetching HMD info!");
		}

		_hmdTrackingState.type = (_internals->systemProperties.trackingProperties.orientationTracking && _internals->systemProperties.trackingProperties.positionTracking)? VRHMDTrackingState::Type::SixDegreesOfFreedom : VRHMDTrackingState::Type::ThreeDegreesOfFreedom;
		RNInfo(GetHMDInfoDescription());

		if(std::strcmp(_internals->systemProperties.systemName, "Oculus Quest2") == 0)
		{
			_deviceType = DeviceType::OculusQuest2;
		}
		else if(std::strcmp(_internals->systemProperties.systemName, "Oculus Quest") == 0)
		{
			_deviceType = DeviceType::OculusQuest2;
		}
		else
		{
			_deviceType = DeviceType::OculusVR;
		}

		InitializeInput();

#if XR_USE_GRAPHICS_API_VULKAN
		if(_supportsVulkan)
		{
			//vulkan_enable2
			//TODO: (there is also a vulkan_enable2 extension, not supported by quest)
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction*)(&_internals->GetVulkanInstanceExtensionsKHR))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction*)(&_internals->GetVulkanDeviceExtensionsKHR))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction*)(&_internals->GetVulkanGraphicsDeviceKHR))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&_internals->GetVulkanGraphicsRequirementsKHR))))
			{

			}
		}
#endif

#if XR_USE_GRAPHICS_API_D3D12
		if(_supportsD3D12)
		{
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&_internals->GetD3D12GraphicsRequirementsKHR))))
			{

			}
		}
#endif

		if(_supportsPreferredFramerate)
		{
			//XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction*)(&_internals->EnumerateDisplayRefreshRatesFB))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&_internals->GetDisplayRefreshRateFB))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&_internals->RequestDisplayRefreshRateFB))))
			{

			}
		}

		if(_supportsPreferredFramerate)
		{
			//XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrPerfSettingsSetPerformanceLevelEXT", (PFN_xrVoidFunction*)(&_internals->PerfSettingsSetPerformanceLevelEXT))))
			{

			}
		}

		if(_supportsFoveatedRendering)
		{
			//XR_FB_FOVEATION_EXTENSION_NAME
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrCreateFoveationProfileFB", (PFN_xrVoidFunction*)(&_internals->CreateFoveationProfileFB))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrDestroyFoveationProfileFB", (PFN_xrVoidFunction*)(&_internals->DestroyFoveationProfileFB))))
			{

			}

			//XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrUpdateSwapchainFB", (PFN_xrVoidFunction*)(&_internals->UpdateSwapchainFB))))
			{

			}

			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrGetSwapchainStateFB", (PFN_xrVoidFunction*)(&_internals->GetSwapchainStateFB))))
			{

			}
		}

#if XR_USE_PLATFORM_ANDROID
		if(_supportsAndroidThreadType)
		{
			//XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME
			if(!XR_SUCCEEDED(xrGetInstanceProcAddr(_internals->instance, "xrSetAndroidApplicationThreadKHR", (PFN_xrVoidFunction*)(&_internals->SetAndroidApplicationThreadKHR))))
			{

			}
		}
#endif
	}

	OpenXRWindow::~OpenXRWindow()
	{
		StopRendering();
		xrDestroyInstance(_internals->instance);
		delete _internals;
	}

	void OpenXRWindow::InitializeInput()
	{
		XrActionSetCreateInfo actionSetInfo;
		actionSetInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
		actionSetInfo.next = nullptr;
		strcpy(actionSetInfo.actionSetName, "game");
		strcpy(actionSetInfo.localizedActionSetName, "Game");
		actionSetInfo.priority = 0;
		if(!XR_SUCCEEDED(xrCreateActionSet(_internals->instance, &actionSetInfo, &_internals->gameActionSet)))
		{
			RN_ASSERT(false, "failed creating action set");
		}

		//Left hand
		XrActionCreateInfo handLeftAimPoseActionInfo;
		handLeftAimPoseActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftAimPoseActionInfo.next = nullptr;
		strcpy(handLeftAimPoseActionInfo.actionName, "hand_left_aim");
		handLeftAimPoseActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(handLeftAimPoseActionInfo.localizedActionName, "Hand Left Aim");
		handLeftAimPoseActionInfo.countSubactionPaths = 0;
		handLeftAimPoseActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftAimPoseActionInfo, &_internals->handLeftAimPoseAction)))
		{
			RN_ASSERT(false, "failed creating left hand aim pose action");
		}

		XrActionCreateInfo handLeftGripPoseActionInfo;
		handLeftGripPoseActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftGripPoseActionInfo.next = nullptr;
		strcpy(handLeftGripPoseActionInfo.actionName, "hand_left_grip");
		handLeftGripPoseActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(handLeftGripPoseActionInfo.localizedActionName, "Hand Left Grip");
		handLeftGripPoseActionInfo.countSubactionPaths = 0;
		handLeftGripPoseActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftGripPoseActionInfo, &_internals->handLeftGripPoseAction)))
		{
			RN_ASSERT(false, "failed creating left hand grip pose action");
		}

		XrActionCreateInfo handLeftTriggerActionInfo;
		handLeftTriggerActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftTriggerActionInfo.next = nullptr;
		strcpy(handLeftTriggerActionInfo.actionName, "hand_left_trigger");
		handLeftTriggerActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handLeftTriggerActionInfo.localizedActionName, "Hand Left Trigger");
		handLeftTriggerActionInfo.countSubactionPaths = 0;
		handLeftTriggerActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftTriggerActionInfo, &_internals->handLeftTriggerAction)))
		{
			RN_ASSERT(false, "failed creating left hand trigger action");
		}

		XrActionCreateInfo handLeftGrabActionInfo;
		handLeftGrabActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftGrabActionInfo.next = nullptr;
		strcpy(handLeftGrabActionInfo.actionName, "hand_left_grab");
		handLeftGrabActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handLeftGrabActionInfo.localizedActionName, "Hand Left Grab");
		handLeftGrabActionInfo.countSubactionPaths = 0;
		handLeftGrabActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftGrabActionInfo, &_internals->handLeftGrabAction)))
		{
			RN_ASSERT(false, "failed creating left hand grab action");
		}

		XrActionCreateInfo handLeftThumbstickXActionInfo;
		handLeftThumbstickXActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftThumbstickXActionInfo.next = nullptr;
		strcpy(handLeftThumbstickXActionInfo.actionName, "hand_left_thumbstick_x");
		handLeftThumbstickXActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handLeftThumbstickXActionInfo.localizedActionName, "Hand Left Thumbstick X");
		handLeftThumbstickXActionInfo.countSubactionPaths = 0;
		handLeftThumbstickXActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftThumbstickXActionInfo, &_internals->handLeftThumbstickXAction)))
		{
			RN_ASSERT(false, "failed creating left hand thumbstick x action");
		}

		XrActionCreateInfo handLeftThumbstickYActionInfo;
		handLeftThumbstickYActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftThumbstickYActionInfo.next = nullptr;
		strcpy(handLeftThumbstickYActionInfo.actionName, "hand_left_thumbstick_y");
		handLeftThumbstickYActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handLeftThumbstickYActionInfo.localizedActionName, "Hand Left Thumbstick Y");
		handLeftThumbstickYActionInfo.countSubactionPaths = 0;
		handLeftThumbstickYActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftThumbstickYActionInfo, &_internals->handLeftThumbstickYAction)))
		{
			RN_ASSERT(false, "failed creating left hand thumbstick y action");
		}

		XrActionCreateInfo handLeftThumbstickPressActionInfo;
		handLeftThumbstickPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftThumbstickPressActionInfo.next = nullptr;
		strcpy(handLeftThumbstickPressActionInfo.actionName, "hand_left_thumbstick_press");
		handLeftThumbstickPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handLeftThumbstickPressActionInfo.localizedActionName, "Hand Left Thumbstick Press");
		handLeftThumbstickPressActionInfo.countSubactionPaths = 0;
		handLeftThumbstickPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftThumbstickPressActionInfo, &_internals->handLeftThumbstickPressAction)))
		{
			RN_ASSERT(false, "failed creating left hand thumbstick press action");
		}

		XrActionCreateInfo handLeftTrackpadXActionInfo;
		handLeftTrackpadXActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftTrackpadXActionInfo.next = nullptr;
		strcpy(handLeftTrackpadXActionInfo.actionName, "hand_left_trackpad_x");
		handLeftTrackpadXActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handLeftTrackpadXActionInfo.localizedActionName, "Hand Left Trackpad X");
		handLeftTrackpadXActionInfo.countSubactionPaths = 0;
		handLeftTrackpadXActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftTrackpadXActionInfo, &_internals->handLeftTrackpadXAction)))
		{
			RN_ASSERT(false, "failed creating left hand trackpad x action");
		}

		XrActionCreateInfo handLeftTrackpadYActionInfo;
		handLeftTrackpadYActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftTrackpadYActionInfo.next = nullptr;
		strcpy(handLeftTrackpadYActionInfo.actionName, "hand_left_trackpad_y");
		handLeftTrackpadYActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handLeftTrackpadYActionInfo.localizedActionName, "Hand Left Trackpad Y");
		handLeftTrackpadYActionInfo.countSubactionPaths = 0;
		handLeftTrackpadYActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftTrackpadYActionInfo, &_internals->handLeftTrackpadYAction)))
		{
			RN_ASSERT(false, "failed creating left hand trackpad y action");
		}

		XrActionCreateInfo handLeftTrackpadTouchActionInfo;
		handLeftTrackpadTouchActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftTrackpadTouchActionInfo.next = nullptr;
		strcpy(handLeftTrackpadTouchActionInfo.actionName, "hand_left_trackpad_touch");
		handLeftTrackpadTouchActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handLeftTrackpadTouchActionInfo.localizedActionName, "Hand Left Trackpad Touch");
		handLeftTrackpadTouchActionInfo.countSubactionPaths = 0;
		handLeftTrackpadTouchActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftTrackpadTouchActionInfo, &_internals->handLeftTrackpadTouchAction)))
		{
			RN_ASSERT(false, "failed creating left hand trackpad touch action");
		}

		XrActionCreateInfo handLeftTrackpadPressActionInfo;
		handLeftTrackpadPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftTrackpadPressActionInfo.next = nullptr;
		strcpy(handLeftTrackpadPressActionInfo.actionName, "hand_left_trackpad_press");
		handLeftTrackpadPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handLeftTrackpadPressActionInfo.localizedActionName, "Hand Left Trackpad Press");
		handLeftTrackpadPressActionInfo.countSubactionPaths = 0;
		handLeftTrackpadPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftTrackpadPressActionInfo, &_internals->handLeftTrackpadPressAction)))
		{
			RN_ASSERT(false, "failed creating left hand trackpad press action");
		}

		XrActionCreateInfo handLeftButtonSystemPressActionInfo;
		handLeftButtonSystemPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftButtonSystemPressActionInfo.next = nullptr;
		strcpy(handLeftButtonSystemPressActionInfo.actionName, "hand_left_button_system_press");
		handLeftButtonSystemPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handLeftButtonSystemPressActionInfo.localizedActionName, "Hand Left Button System Press");
		handLeftButtonSystemPressActionInfo.countSubactionPaths = 0;
		handLeftButtonSystemPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftButtonSystemPressActionInfo, &_internals->handLeftButtonSystemPressAction)))
		{
			RN_ASSERT(false, "failed creating left hand button system press action");
		}

		XrActionCreateInfo handLeftButtonUpperPressActionInfo;
		handLeftButtonUpperPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftButtonUpperPressActionInfo.next = nullptr;
		strcpy(handLeftButtonUpperPressActionInfo.actionName, "hand_left_button_upper_press");
		handLeftButtonUpperPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handLeftButtonUpperPressActionInfo.localizedActionName, "Hand Left Button Upper Press");
		handLeftButtonUpperPressActionInfo.countSubactionPaths = 0;
		handLeftButtonUpperPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftButtonUpperPressActionInfo, &_internals->handLeftButtonUpperPressAction)))
		{
			RN_ASSERT(false, "failed creating left hand button upper press action");
		}

		XrActionCreateInfo handLeftButtonLowerPressActionInfo;
		handLeftButtonLowerPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftButtonLowerPressActionInfo.next = nullptr;
		strcpy(handLeftButtonLowerPressActionInfo.actionName, "hand_left_button_lower_press");
		handLeftButtonLowerPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handLeftButtonLowerPressActionInfo.localizedActionName, "Hand Left Button Lower Press");
		handLeftButtonLowerPressActionInfo.countSubactionPaths = 0;
		handLeftButtonLowerPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftButtonLowerPressActionInfo, &_internals->handLeftButtonLowerPressAction)))
		{
			RN_ASSERT(false, "failed creating left hand button lower press action");
		}

		XrActionCreateInfo handLeftHapticsActionInfo;
		handLeftHapticsActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftHapticsActionInfo.next = nullptr;
		strcpy(handLeftHapticsActionInfo.actionName, "hand_left_haptics");
		handLeftHapticsActionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
		strcpy(handLeftHapticsActionInfo.localizedActionName, "Hand Left Haptics");
		handLeftHapticsActionInfo.countSubactionPaths = 0;
		handLeftHapticsActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftHapticsActionInfo, &_internals->handLeftHapticsAction)))
		{
			RN_ASSERT(false, "failed creating left hand haptics action");
		}

		//Right hand
		XrActionCreateInfo handRightAimPoseActionInfo;
		handRightAimPoseActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightAimPoseActionInfo.next = nullptr;
		strcpy(handRightAimPoseActionInfo.actionName, "hand_right_aim");
		handRightAimPoseActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(handRightAimPoseActionInfo.localizedActionName, "Hand Right Aim");
		handRightAimPoseActionInfo.countSubactionPaths = 0;
		handRightAimPoseActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightAimPoseActionInfo, &_internals->handRightAimPoseAction)))
		{
			RN_ASSERT(false, "failed creating right hand aim pose action");
		}

		XrActionCreateInfo handRightGripPoseActionInfo;
		handRightGripPoseActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightGripPoseActionInfo.next = nullptr;
		strcpy(handRightGripPoseActionInfo.actionName, "hand_right_grip");
		handRightGripPoseActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(handRightGripPoseActionInfo.localizedActionName, "Hand Right Grip");
		handRightGripPoseActionInfo.countSubactionPaths = 0;
		handRightGripPoseActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightGripPoseActionInfo, &_internals->handRightGripPoseAction)))
		{
			RN_ASSERT(false, "failed creating right hand grip pose action");
		}

		XrActionCreateInfo handRightTriggerActionInfo;
		handRightTriggerActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightTriggerActionInfo.next = nullptr;
		strcpy(handRightTriggerActionInfo.actionName, "hand_right_trigger");
		handRightTriggerActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handRightTriggerActionInfo.localizedActionName, "Hand Right Trigger");
		handRightTriggerActionInfo.countSubactionPaths = 0;
		handRightTriggerActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightTriggerActionInfo, &_internals->handRightTriggerAction)))
		{
			RN_ASSERT(false, "failed creating right hand trigger action");
		}

		XrActionCreateInfo handRightGrabActionInfo;
		handRightGrabActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightGrabActionInfo.next = nullptr;
		strcpy(handRightGrabActionInfo.actionName, "hand_right_grab");
		handRightGrabActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handRightGrabActionInfo.localizedActionName, "Hand Right Grab");
		handRightGrabActionInfo.countSubactionPaths = 0;
		handRightGrabActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightGrabActionInfo, &_internals->handRightGrabAction)))
		{
			RN_ASSERT(false, "failed creating right hand grab action");
		}

		XrActionCreateInfo handRightThumbstickXActionInfo;
		handRightThumbstickXActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightThumbstickXActionInfo.next = nullptr;
		strcpy(handRightThumbstickXActionInfo.actionName, "hand_right_thumbstick_x");
		handRightThumbstickXActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handRightThumbstickXActionInfo.localizedActionName, "Hand Right Thumbstick X");
		handRightThumbstickXActionInfo.countSubactionPaths = 0;
		handRightThumbstickXActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightThumbstickXActionInfo, &_internals->handRightThumbstickXAction)))
		{
			RN_ASSERT(false, "failed creating right hand thumbstick x action");
		}

		XrActionCreateInfo handRightThumbstickYActionInfo;
		handRightThumbstickYActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightThumbstickYActionInfo.next = nullptr;
		strcpy(handRightThumbstickYActionInfo.actionName, "hand_right_thumbstick_y");
		handRightThumbstickYActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handRightThumbstickYActionInfo.localizedActionName, "Hand Right Thumbstick Y");
		handRightThumbstickYActionInfo.countSubactionPaths = 0;
		handRightThumbstickYActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightThumbstickYActionInfo, &_internals->handRightThumbstickYAction)))
		{
			RN_ASSERT(false, "failed creating right hand thumbstick y action");
		}

		XrActionCreateInfo handRightThumbstickPressActionInfo;
		handRightThumbstickPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightThumbstickPressActionInfo.next = nullptr;
		strcpy(handRightThumbstickPressActionInfo.actionName, "hand_right_thumbstick_press");
		handRightThumbstickPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handRightThumbstickPressActionInfo.localizedActionName, "Hand Right Thumbstick Press");
		handRightThumbstickPressActionInfo.countSubactionPaths = 0;
		handRightThumbstickPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightThumbstickPressActionInfo, &_internals->handRightThumbstickPressAction)))
		{
			RN_ASSERT(false, "failed creating right hand thumbstick press action");
		}

		XrActionCreateInfo handRightTrackpadXActionInfo;
		handRightTrackpadXActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightTrackpadXActionInfo.next = nullptr;
		strcpy(handRightTrackpadXActionInfo.actionName, "hand_right_trackpad_x");
		handRightTrackpadXActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handRightTrackpadXActionInfo.localizedActionName, "Hand Right Trackpad X");
		handRightTrackpadXActionInfo.countSubactionPaths = 0;
		handRightTrackpadXActionInfo.subactionPaths = nullptr;
		if (!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightTrackpadXActionInfo, &_internals->handRightTrackpadXAction)))
		{
			RN_ASSERT(false, "failed creating right hand trackpad x action");
		}

		XrActionCreateInfo handRightTrackpadYActionInfo;
		handRightTrackpadYActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightTrackpadYActionInfo.next = nullptr;
		strcpy(handRightTrackpadYActionInfo.actionName, "hand_right_trackpad_y");
		handRightTrackpadYActionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(handRightTrackpadYActionInfo.localizedActionName, "Hand Right Trackpad Y");
		handRightTrackpadYActionInfo.countSubactionPaths = 0;
		handRightTrackpadYActionInfo.subactionPaths = nullptr;
		if (!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightTrackpadYActionInfo, &_internals->handRightTrackpadYAction)))
		{
			RN_ASSERT(false, "failed creating right hand trackpad y action");
		}

		XrActionCreateInfo handRightTrackpadTouchActionInfo;
		handRightTrackpadTouchActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightTrackpadTouchActionInfo.next = nullptr;
		strcpy(handRightTrackpadTouchActionInfo.actionName, "hand_right_trackpad_touch");
		handRightTrackpadTouchActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handRightTrackpadTouchActionInfo.localizedActionName, "Hand Right Trackpad Touch");
		handRightTrackpadTouchActionInfo.countSubactionPaths = 0;
		handRightTrackpadTouchActionInfo.subactionPaths = nullptr;
		if (!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightTrackpadTouchActionInfo, &_internals->handRightTrackpadTouchAction)))
		{
			RN_ASSERT(false, "failed creating right hand trackpad touch action");
		}

		XrActionCreateInfo handRightTrackpadPressActionInfo;
		handRightTrackpadPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightTrackpadPressActionInfo.next = nullptr;
		strcpy(handRightTrackpadPressActionInfo.actionName, "hand_right_trackpad_press");
		handRightTrackpadPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handRightTrackpadPressActionInfo.localizedActionName, "Hand Right Trackpad Press");
		handRightTrackpadPressActionInfo.countSubactionPaths = 0;
		handRightTrackpadPressActionInfo.subactionPaths = nullptr;
		if (!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightTrackpadPressActionInfo, &_internals->handRightTrackpadPressAction)))
		{
			RN_ASSERT(false, "failed creating right hand trackpad press action");
		}

		XrActionCreateInfo handRightButtonSystemPressActionInfo;
		handRightButtonSystemPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightButtonSystemPressActionInfo.next = nullptr;
		strcpy(handRightButtonSystemPressActionInfo.actionName, "hand_right_button_system_press");
		handRightButtonSystemPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handRightButtonSystemPressActionInfo.localizedActionName, "Hand Right Button System Press");
		handRightButtonSystemPressActionInfo.countSubactionPaths = 0;
		handRightButtonSystemPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightButtonSystemPressActionInfo, &_internals->handRightButtonSystemPressAction)))
		{
			RN_ASSERT(false, "failed creating right hand button system press action");
		}

		XrActionCreateInfo handRightButtonUpperPressActionInfo;
		handRightButtonUpperPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightButtonUpperPressActionInfo.next = nullptr;
		strcpy(handRightButtonUpperPressActionInfo.actionName, "hand_right_button_upper_press");
		handRightButtonUpperPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handRightButtonUpperPressActionInfo.localizedActionName, "Hand Right Button Upper Press");
		handRightButtonUpperPressActionInfo.countSubactionPaths = 0;
		handRightButtonUpperPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightButtonUpperPressActionInfo, &_internals->handRightButtonUpperPressAction)))
		{
			RN_ASSERT(false, "failed creating right hand button upper press action");
		}

		XrActionCreateInfo handRightButtonLowerPressActionInfo;
		handRightButtonLowerPressActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightButtonLowerPressActionInfo.next = nullptr;
		strcpy(handRightButtonLowerPressActionInfo.actionName, "hand_right_button_lower_press");
		handRightButtonLowerPressActionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(handRightButtonLowerPressActionInfo.localizedActionName, "Hand Right Button Lower Press");
		handRightButtonLowerPressActionInfo.countSubactionPaths = 0;
		handRightButtonLowerPressActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightButtonLowerPressActionInfo, &_internals->handRightButtonLowerPressAction)))
		{
			RN_ASSERT(false, "failed creating right hand button lower press action");
		}

		XrActionCreateInfo handRightHapticsActionInfo;
		handRightHapticsActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handRightHapticsActionInfo.next = nullptr;
		strcpy(handRightHapticsActionInfo.actionName, "hand_right_haptics");
		handRightHapticsActionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
		strcpy(handRightHapticsActionInfo.localizedActionName, "Hand Right Haptics");
		handRightHapticsActionInfo.countSubactionPaths = 0;
		handRightHapticsActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightHapticsActionInfo, &_internals->handRightHapticsAction)))
		{
			RN_ASSERT(false, "failed creating right hand haptics action");
		}

		//Suggested controller bindings
		XrPath handLeftAimPosePath;
		XrPath handLeftGripPosePath;
		XrPath handLeftTriggerPath;
		XrPath handLeftGrabPath;
		XrPath handLeftThumbstickXPath;
		XrPath handLeftThumbstickYPath;
		XrPath handLeftThumbstickPressPath;
		XrPath handLeftTrackpadXPath;
		XrPath handLeftTrackpadYPath;
		XrPath handLeftTrackpadTouchPath;
		XrPath handLeftTrackpadPressPath;
		XrPath handLeftButtonSystemPressPath;
		XrPath handLeftButtonUpperPressPath;
		XrPath handLeftButtonLowerPressPath;
		XrPath handLeftHapticsPath;

		XrPath handRightAimPosePath;
		XrPath handRightGripPosePath;
		XrPath handRightTriggerPath;
		XrPath handRightGrabPath;
		XrPath handRightThumbstickXPath;
		XrPath handRightThumbstickYPath;
		XrPath handRightThumbstickPressPath;
		XrPath handRightTrackpadXPath;
		XrPath handRightTrackpadYPath;
		XrPath handRightTrackpadTouchPath;
		XrPath handRightTrackpadPressPath;
		XrPath handRightButtonSystemPressPath;
		XrPath handRightButtonUpperPressPath;
		XrPath handRightButtonLowerPressPath;
		XrPath handRightHapticsPath;

		XrPath interactionProfilePath;


		//Simple controller
		//Left hand
		xrStringToPath(_internals->instance, "/user/hand/left/input/aim/pose", &handLeftAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/grip/pose", &handLeftGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/select/click", &handLeftTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/menu/click", &handLeftButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/output/haptic", &handLeftHapticsPath);

		//Right hand
		xrStringToPath(_internals->instance, "/user/hand/right/input/aim/pose", &handRightAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/grip/pose", &handRightGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/select/click", &handRightTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/menu/click", &handRightButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/output/haptic", &handRightHapticsPath);

		std::vector<XrActionSuggestedBinding> simpleControllerBindings;
		simpleControllerBindings.push_back({ _internals->handLeftAimPoseAction, handLeftAimPosePath });
		simpleControllerBindings.push_back({ _internals->handLeftGripPoseAction, handLeftGripPosePath });
		simpleControllerBindings.push_back({ _internals->handLeftTriggerAction, handLeftTriggerPath });
		simpleControllerBindings.push_back({ _internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath });
		simpleControllerBindings.push_back({ _internals->handLeftHapticsAction, handLeftHapticsPath });

		simpleControllerBindings.push_back({ _internals->handRightAimPoseAction, handRightAimPosePath });
		simpleControllerBindings.push_back({ _internals->handRightGripPoseAction, handRightGripPosePath });
		simpleControllerBindings.push_back({ _internals->handRightTriggerAction, handRightTriggerPath });
		simpleControllerBindings.push_back({ _internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath });
		simpleControllerBindings.push_back({ _internals->handRightHapticsAction, handRightHapticsPath });

		xrStringToPath(_internals->instance, "/interaction_profiles/khr/simple_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedSimpleBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedSimpleBindings.interactionProfile = interactionProfilePath;
		suggestedSimpleBindings.suggestedBindings = simpleControllerBindings.data();
		suggestedSimpleBindings.countSuggestedBindings = simpleControllerBindings.size();
		if (!XR_SUCCEEDED(xrSuggestInteractionProfileBindings(_internals->instance, &suggestedSimpleBindings)))
		{
			RNDebug("failed action profile suggested simple controller binding");
		}


		//Oculus touch bindings
		//Left hand
		xrStringToPath(_internals->instance, "/user/hand/left/input/aim/pose", &handLeftAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/grip/pose", &handLeftGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trigger/value", &handLeftTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/squeeze/value", &handLeftGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/x", &handLeftThumbstickXPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/y", &handLeftThumbstickYPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/click", &handLeftThumbstickPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/menu/click", &handLeftButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/y/click", &handLeftButtonUpperPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/x/click", &handLeftButtonLowerPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/output/haptic", &handLeftHapticsPath);

		//Right hand
		xrStringToPath(_internals->instance, "/user/hand/right/input/aim/pose", &handRightAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/grip/pose", &handRightGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trigger/value", &handRightTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/squeeze/value", &handRightGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/x", &handRightThumbstickXPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/y", &handRightThumbstickYPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/click", &handRightThumbstickPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/system/click", &handRightButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/b/click", &handRightButtonUpperPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/a/click", &handRightButtonLowerPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/output/haptic", &handRightHapticsPath);

		std::vector<XrActionSuggestedBinding> oculusTouchBindings;
		oculusTouchBindings.push_back({_internals->handLeftAimPoseAction, handLeftAimPosePath});
		oculusTouchBindings.push_back({_internals->handLeftGripPoseAction, handLeftGripPosePath});
		oculusTouchBindings.push_back({_internals->handLeftTriggerAction, handLeftTriggerPath});
		oculusTouchBindings.push_back({_internals->handLeftGrabAction, handLeftGrabPath});
		oculusTouchBindings.push_back({_internals->handLeftThumbstickXAction, handLeftThumbstickXPath});
		oculusTouchBindings.push_back({_internals->handLeftThumbstickYAction, handLeftThumbstickYPath});
		oculusTouchBindings.push_back({_internals->handLeftThumbstickPressAction, handLeftThumbstickPressPath});
		oculusTouchBindings.push_back({_internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath});
		oculusTouchBindings.push_back({_internals->handLeftButtonUpperPressAction, handLeftButtonUpperPressPath});
		oculusTouchBindings.push_back({_internals->handLeftButtonLowerPressAction, handLeftButtonLowerPressPath});
		oculusTouchBindings.push_back({_internals->handLeftHapticsAction, handLeftHapticsPath});

		oculusTouchBindings.push_back({_internals->handRightAimPoseAction, handRightAimPosePath});
		oculusTouchBindings.push_back({_internals->handRightGripPoseAction, handRightGripPosePath});
		oculusTouchBindings.push_back({_internals->handRightTriggerAction, handRightTriggerPath});
		oculusTouchBindings.push_back({_internals->handRightGrabAction, handRightGrabPath});
		oculusTouchBindings.push_back({_internals->handRightThumbstickXAction, handRightThumbstickXPath});
		oculusTouchBindings.push_back({_internals->handRightThumbstickYAction, handRightThumbstickYPath});
		oculusTouchBindings.push_back({_internals->handRightThumbstickPressAction, handRightThumbstickPressPath});
		oculusTouchBindings.push_back({_internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath});
		oculusTouchBindings.push_back({_internals->handRightButtonUpperPressAction, handRightButtonUpperPressPath});
		oculusTouchBindings.push_back({_internals->handRightButtonLowerPressAction, handRightButtonLowerPressPath});
		oculusTouchBindings.push_back({_internals->handRightHapticsAction, handRightHapticsPath});

		xrStringToPath(_internals->instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = oculusTouchBindings.data();
		suggestedBindings.countSuggestedBindings = oculusTouchBindings.size();
		if(!XR_SUCCEEDED(xrSuggestInteractionProfileBindings(_internals->instance, &suggestedBindings)))
		{
			RNDebug("failed action profile suggested binding");
		}


		//Vive wand bindings
		//Left hand
		xrStringToPath(_internals->instance, "/user/hand/left/input/aim/pose", &handLeftAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/grip/pose", &handLeftGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trigger/value", &handLeftTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/squeeze/click", &handLeftGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/x", &handLeftTrackpadXPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/y", &handLeftTrackpadYPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/touch", &handLeftTrackpadTouchPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/click", &handLeftTrackpadPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/menu/click", &handLeftButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/output/haptic", &handLeftHapticsPath);

		//Right hand
		xrStringToPath(_internals->instance, "/user/hand/right/input/aim/pose", &handRightAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/grip/pose", &handRightGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trigger/value", &handRightTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/squeeze/click", &handRightGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/x", &handRightTrackpadXPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/y", &handRightTrackpadYPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/touch", &handRightTrackpadTouchPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/click", &handRightTrackpadPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/menu/click", &handRightButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/output/haptic", &handRightHapticsPath);

		std::vector<XrActionSuggestedBinding> viveWandBindings;
		viveWandBindings.push_back({ _internals->handLeftAimPoseAction, handLeftAimPosePath });
		viveWandBindings.push_back({ _internals->handLeftGripPoseAction, handLeftGripPosePath });
		viveWandBindings.push_back({ _internals->handLeftTriggerAction, handLeftTriggerPath });
		viveWandBindings.push_back({ _internals->handLeftGrabAction, handLeftGrabPath });
		viveWandBindings.push_back({ _internals->handLeftTrackpadXAction, handLeftTrackpadXPath });
		viveWandBindings.push_back({ _internals->handLeftTrackpadYAction, handLeftTrackpadYPath });
		viveWandBindings.push_back({ _internals->handLeftTrackpadTouchAction, handLeftTrackpadTouchPath });
		viveWandBindings.push_back({ _internals->handLeftTrackpadPressAction, handLeftTrackpadPressPath });
		viveWandBindings.push_back({ _internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath });
		viveWandBindings.push_back({ _internals->handLeftHapticsAction, handLeftHapticsPath });

		viveWandBindings.push_back({ _internals->handRightAimPoseAction, handRightAimPosePath });
		viveWandBindings.push_back({ _internals->handRightGripPoseAction, handRightGripPosePath });
		viveWandBindings.push_back({ _internals->handRightTriggerAction, handRightTriggerPath });
		viveWandBindings.push_back({ _internals->handRightGrabAction, handRightGrabPath });
		viveWandBindings.push_back({ _internals->handRightTrackpadXAction, handRightTrackpadXPath });
		viveWandBindings.push_back({ _internals->handRightTrackpadYAction, handRightTrackpadYPath });
		viveWandBindings.push_back({ _internals->handRightTrackpadTouchAction, handRightTrackpadTouchPath });
		viveWandBindings.push_back({ _internals->handRightTrackpadPressAction, handRightTrackpadPressPath });
		viveWandBindings.push_back({ _internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath });
		viveWandBindings.push_back({ _internals->handRightHapticsAction, handRightHapticsPath });

		xrStringToPath(_internals->instance, "/interaction_profiles/htc/vive_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedViveWandBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedViveWandBindings.interactionProfile = interactionProfilePath;
		suggestedViveWandBindings.suggestedBindings = viveWandBindings.data();
		suggestedViveWandBindings.countSuggestedBindings = viveWandBindings.size();
		if(!XR_SUCCEEDED(xrSuggestInteractionProfileBindings(_internals->instance, &suggestedViveWandBindings)))
		{
			RNDebug("failed action profile suggested vive wand binding");
		}


		//Microsoft mixed reality controller bindings
		//Left hand
		xrStringToPath(_internals->instance, "/user/hand/left/input/aim/pose", &handLeftAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/grip/pose", &handLeftGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trigger/value", &handLeftTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/squeeze/click", &handLeftGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/x", &handLeftThumbstickXPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/y", &handLeftThumbstickYPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/click", &handLeftThumbstickPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/x", &handLeftTrackpadXPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/y", &handLeftTrackpadYPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/touch", &handLeftTrackpadTouchPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/click", &handLeftTrackpadPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/menu/click", &handLeftButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/output/haptic", &handLeftHapticsPath);

		//Right hand
		xrStringToPath(_internals->instance, "/user/hand/right/input/aim/pose", &handRightAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/grip/pose", &handRightGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trigger/value", &handRightTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/squeeze/click", &handRightGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/x", &handRightThumbstickXPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/y", &handRightThumbstickYPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/click", &handRightThumbstickPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/x", &handRightTrackpadXPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/y", &handRightTrackpadYPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/touch", &handRightTrackpadTouchPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/click", &handRightTrackpadPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/menu/click", &handRightButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/output/haptic", &handRightHapticsPath);

		std::vector<XrActionSuggestedBinding> microsoftMixedRealityBindings;
		microsoftMixedRealityBindings.push_back({ _internals->handLeftAimPoseAction, handLeftAimPosePath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftGripPoseAction, handLeftGripPosePath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftTriggerAction, handLeftTriggerPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftGrabAction, handLeftGrabPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftThumbstickXAction, handLeftThumbstickXPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftThumbstickYAction, handLeftThumbstickYPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftThumbstickPressAction, handLeftThumbstickPressPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftTrackpadXAction, handLeftTrackpadXPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftTrackpadYAction, handLeftTrackpadYPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftTrackpadTouchAction, handLeftTrackpadTouchPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftTrackpadPressAction, handLeftTrackpadPressPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath });
		microsoftMixedRealityBindings.push_back({ _internals->handLeftHapticsAction, handLeftHapticsPath });

		microsoftMixedRealityBindings.push_back({ _internals->handRightAimPoseAction, handRightAimPosePath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightGripPoseAction, handRightGripPosePath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightTriggerAction, handRightTriggerPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightGrabAction, handRightGrabPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightThumbstickXAction, handRightThumbstickXPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightThumbstickYAction, handRightThumbstickYPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightThumbstickPressAction, handRightThumbstickPressPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightTrackpadXAction, handRightTrackpadXPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightTrackpadYAction, handRightTrackpadYPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightTrackpadTouchAction, handRightTrackpadTouchPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightTrackpadPressAction, handRightTrackpadPressPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath });
		microsoftMixedRealityBindings.push_back({ _internals->handRightHapticsAction, handRightHapticsPath });

		xrStringToPath(_internals->instance, "/interaction_profiles/microsoft/motion_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedMicrosoftMixedRealityBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedMicrosoftMixedRealityBindings.interactionProfile = interactionProfilePath;
		suggestedMicrosoftMixedRealityBindings.suggestedBindings = microsoftMixedRealityBindings.data();
		suggestedMicrosoftMixedRealityBindings.countSuggestedBindings = microsoftMixedRealityBindings.size();
		if (!XR_SUCCEEDED(xrSuggestInteractionProfileBindings(_internals->instance, &suggestedMicrosoftMixedRealityBindings)))
		{
			RNDebug("failed action profile suggested microsoft mixed reality controller binding");
		}


		//Valve Index bindings
		//Left hand
		xrStringToPath(_internals->instance, "/user/hand/left/input/aim/pose", &handLeftAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/grip/pose", &handLeftGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trigger/value", &handLeftTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/squeeze/value", &handLeftGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/x", &handLeftThumbstickXPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/y", &handLeftThumbstickYPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/click", &handLeftThumbstickPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/x", &handLeftTrackpadXPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/y", &handLeftTrackpadYPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/touch", &handLeftTrackpadTouchPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/trackpad/force", &handLeftTrackpadPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/system/click", &handLeftButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/b/click", &handLeftButtonUpperPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/input/a/click", &handLeftButtonLowerPressPath);
		xrStringToPath(_internals->instance, "/user/hand/left/output/haptic", &handLeftHapticsPath);

		//Right hand
		xrStringToPath(_internals->instance, "/user/hand/right/input/aim/pose", &handRightAimPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/grip/pose", &handRightGripPosePath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trigger/value", &handRightTriggerPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/squeeze/value", &handRightGrabPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/x", &handRightThumbstickXPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/y", &handRightThumbstickYPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/click", &handRightThumbstickPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/x", &handRightTrackpadXPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/y", &handRightTrackpadYPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/touch", &handRightTrackpadTouchPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/trackpad/force", &handRightTrackpadPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/system/click", &handRightButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/b/click", &handRightButtonUpperPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/a/click", &handRightButtonLowerPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/output/haptic", &handRightHapticsPath);

		std::vector<XrActionSuggestedBinding> valveIndexBindings;
		valveIndexBindings.push_back({ _internals->handLeftAimPoseAction, handLeftAimPosePath });
		valveIndexBindings.push_back({ _internals->handLeftGripPoseAction, handLeftGripPosePath });
		valveIndexBindings.push_back({ _internals->handLeftTriggerAction, handLeftTriggerPath });
		valveIndexBindings.push_back({ _internals->handLeftGrabAction, handLeftGrabPath });
		valveIndexBindings.push_back({ _internals->handLeftThumbstickXAction, handLeftThumbstickXPath });
		valveIndexBindings.push_back({ _internals->handLeftThumbstickYAction, handLeftThumbstickYPath });
		valveIndexBindings.push_back({ _internals->handLeftThumbstickPressAction, handLeftThumbstickPressPath });
		valveIndexBindings.push_back({ _internals->handLeftTrackpadXAction, handLeftTrackpadXPath });
		valveIndexBindings.push_back({ _internals->handLeftTrackpadYAction, handLeftTrackpadYPath });
		valveIndexBindings.push_back({ _internals->handLeftTrackpadTouchAction, handLeftTrackpadTouchPath });
		valveIndexBindings.push_back({ _internals->handLeftTrackpadPressAction, handLeftTrackpadPressPath });
		valveIndexBindings.push_back({ _internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath });
		valveIndexBindings.push_back({ _internals->handLeftButtonUpperPressAction, handLeftButtonUpperPressPath });
		valveIndexBindings.push_back({ _internals->handLeftButtonLowerPressAction, handLeftButtonLowerPressPath });
		valveIndexBindings.push_back({ _internals->handLeftHapticsAction, handLeftHapticsPath });

		valveIndexBindings.push_back({ _internals->handRightAimPoseAction, handRightAimPosePath });
		valveIndexBindings.push_back({ _internals->handRightGripPoseAction, handRightGripPosePath });
		valveIndexBindings.push_back({ _internals->handRightTriggerAction, handRightTriggerPath });
		valveIndexBindings.push_back({ _internals->handRightGrabAction, handRightGrabPath });
		valveIndexBindings.push_back({ _internals->handRightThumbstickXAction, handRightThumbstickXPath });
		valveIndexBindings.push_back({ _internals->handRightThumbstickYAction, handRightThumbstickYPath });
		valveIndexBindings.push_back({ _internals->handRightThumbstickPressAction, handRightThumbstickPressPath });
		valveIndexBindings.push_back({ _internals->handRightTrackpadXAction, handRightTrackpadXPath });
		valveIndexBindings.push_back({ _internals->handRightTrackpadYAction, handRightTrackpadYPath });
		valveIndexBindings.push_back({ _internals->handRightTrackpadTouchAction, handRightTrackpadTouchPath });
		valveIndexBindings.push_back({ _internals->handRightTrackpadPressAction, handRightTrackpadPressPath });
		valveIndexBindings.push_back({ _internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath });
		valveIndexBindings.push_back({ _internals->handRightButtonUpperPressAction, handRightButtonUpperPressPath });
		valveIndexBindings.push_back({ _internals->handRightButtonLowerPressAction, handRightButtonLowerPressPath });
		valveIndexBindings.push_back({ _internals->handRightHapticsAction, handRightHapticsPath });

		xrStringToPath(_internals->instance, "/interaction_profiles/valve/index_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedValveIndexBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedValveIndexBindings.interactionProfile = interactionProfilePath;
		suggestedValveIndexBindings.suggestedBindings = valveIndexBindings.data();
		suggestedValveIndexBindings.countSuggestedBindings = valveIndexBindings.size();
		if (!XR_SUCCEEDED(xrSuggestInteractionProfileBindings(_internals->instance, &suggestedValveIndexBindings)))
		{
			RNDebug("failed action profile suggested valve index binding");
		}
	}

	void OpenXRWindow::StartRendering(const SwapChainDescriptor &descriptor, float eyeResolutionFactor)
	{
		XrSessionCreateInfo sessionCreateInfo;
		sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
		sessionCreateInfo.next = nullptr;
		sessionCreateInfo.createFlags = 0;
		sessionCreateInfo.systemId = _internals->systemID;
		
#ifdef XR_USE_GRAPHICS_API_VULKAN
		XrGraphicsBindingVulkanKHR vulkanGraphicsBinding;
		vulkanGraphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
		vulkanGraphicsBinding.next = nullptr;
		
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Vulkan")))
		{
			VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();

			XrGraphicsRequirementsVulkanKHR graphicsRequirements;
			graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR;
			graphicsRequirements.next = nullptr;
			if(!XR_SUCCEEDED(_internals->GetVulkanGraphicsRequirementsKHR(_internals->instance, _internals->systemID, &graphicsRequirements)))
			{
				RN_ASSERT(false, "Failed fetching vulkan graphics requirements");
			}

			RNDebug("Minimum supported vulkan version: " << XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported) << "." << XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported));
			RNDebug("Maximum tested vulkan version: " << XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported) << "." << XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported));

			VkPhysicalDevice physicalDevice;
			if(!XR_SUCCEEDED(_internals->GetVulkanGraphicsDeviceKHR(_internals->instance, _internals->systemID, renderer->GetVulkanInstance()->GetInstance(), &physicalDevice)))
			{
				RN_ASSERT(false, "Failed fetching vulkan graphics device");
			}

			vulkanGraphicsBinding.instance = renderer->GetVulkanInstance()->GetInstance();
			vulkanGraphicsBinding.physicalDevice = renderer->GetVulkanDevice()->GetPhysicalDevice();
			vulkanGraphicsBinding.device = renderer->GetVulkanDevice()->GetDevice();
			vulkanGraphicsBinding.queueFamilyIndex = renderer->GetVulkanDevice()->GetWorkQueue();
			vulkanGraphicsBinding.queueIndex = 0; //There should be only one queue at the moment, so it's index should be 0...

			sessionCreateInfo.next = &vulkanGraphicsBinding;
		}
#endif

#ifdef XR_USE_GRAPHICS_API_D3D12
		XrGraphicsBindingD3D12KHR d3d12GraphicsBinding;
		d3d12GraphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_D3D12_KHR;
		d3d12GraphicsBinding.next = nullptr;

		if (Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("D3D12")))
		{
			D3D12Renderer *renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();

			XrGraphicsRequirementsD3D12KHR graphicsRequirements;
			graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR;
			graphicsRequirements.next = nullptr;
			if(!XR_SUCCEEDED(_internals->GetD3D12GraphicsRequirementsKHR(_internals->instance, _internals->systemID, &graphicsRequirements)))
			{
				RN_ASSERT(false, "Failed fetching d3d12 graphics requirements");
			}

			d3d12GraphicsBinding.device = renderer->GetD3D12Device()->GetDevice();
			d3d12GraphicsBinding.queue = renderer->GetCommandQueue();

			sessionCreateInfo.next = &d3d12GraphicsBinding;
		}
#endif

		uint32_t numberOfConfigurationViews = 0;
		if(!XR_SUCCEEDED(xrEnumerateViewConfigurationViews(_internals->instance, _internals->systemID, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &numberOfConfigurationViews, nullptr)))
		{

		}

		XrViewConfigurationView *configurationViews = new XrViewConfigurationView[numberOfConfigurationViews];
		for(int i = 0; i < numberOfConfigurationViews; i++)
		{
			configurationViews[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
			configurationViews[i].next = nullptr;
		}
		
		if(!XR_SUCCEEDED(xrEnumerateViewConfigurationViews(_internals->instance, _internals->systemID, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, numberOfConfigurationViews, &numberOfConfigurationViews, configurationViews)))
		{

		}

		for(int i = 0; i < numberOfConfigurationViews; i++)
		{
			RNDebug("View: " << configurationViews[i].recommendedImageRectWidth << " x " << configurationViews[i].recommendedImageRectHeight << " : " << configurationViews[i].recommendedSwapchainSampleCount);
		}

		Vector2 eyeRenderSize(configurationViews[0].recommendedImageRectWidth * eyeResolutionFactor, configurationViews[0].recommendedImageRectHeight * eyeResolutionFactor);
		delete[] configurationViews;
		
		if(!XR_SUCCEEDED(xrCreateSession(_internals->instance, &sessionCreateInfo, &_internals->session)))
		{
			RN_ASSERT(false, "failed creating session");
		}

		XrReferenceSpaceCreateInfo referenceSpaceCreateInfo;
		referenceSpaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		referenceSpaceCreateInfo.next = nullptr;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
		referenceSpaceCreateInfo.poseInReferenceSpace.position.x = 0.0f;
		referenceSpaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
		referenceSpaceCreateInfo.poseInReferenceSpace.position.z = 0.0f;
		referenceSpaceCreateInfo.poseInReferenceSpace.orientation.x = 0.0f;
		referenceSpaceCreateInfo.poseInReferenceSpace.orientation.y = 0.0f;
		referenceSpaceCreateInfo.poseInReferenceSpace.orientation.z = 0.0f;
		referenceSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;

		xrCreateReferenceSpace(_internals->session, &referenceSpaceCreateInfo, &_internals->trackingSpace);

		_internals->views = new XrView[2];
		_internals->views[0].type = XR_TYPE_VIEW;
		_internals->views[0].next = nullptr;
		_internals->views[1].type = XR_TYPE_VIEW;
		_internals->views[1].next = nullptr;

#ifdef XR_USE_GRAPHICS_API_VULKAN
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("Vulkan")))
		{
			_swapChain = new OpenXRVulkanSwapChain(this, descriptor, eyeRenderSize);
			_swapChainType = SwapChainType::Vulkan;
		}
#endif
#ifdef XR_USE_GRAPHICS_API_D3D12
		if(Renderer::GetActiveRenderer()->GetDescriptor()->GetAPI()->IsEqual(RNCSTR("D3D12")))
		{
			_swapChain = new OpenXRD3D12SwapChain(this, descriptor, eyeRenderSize);
			_swapChainType = SwapChainType::D3D12;
		}
#endif

		_swapChain->_presentEvent = [this](){
			if(_internals->session != XR_NULL_HANDLE && _isSessionRunning)
			{
				XrCompositionLayerProjectionView layerProjectionView[2];
				layerProjectionView[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
				layerProjectionView[0].next = nullptr;
				layerProjectionView[0].pose = _internals->views[0].pose;
				layerProjectionView[0].fov = _internals->views[0].fov;
				layerProjectionView[0].subImage.swapchain = _swapChain->_internals->swapchain;
				layerProjectionView[0].subImage.imageRect.offset.x = 0;
				layerProjectionView[0].subImage.imageRect.offset.y = 0;
				layerProjectionView[0].subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
				layerProjectionView[0].subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
				layerProjectionView[0].subImage.imageArrayIndex = 0;

				layerProjectionView[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
				layerProjectionView[1].next = nullptr;
				layerProjectionView[1].pose = _internals->views[1].pose;
				layerProjectionView[1].fov = _internals->views[1].fov;
				layerProjectionView[1].subImage.swapchain = _swapChain->_internals->swapchain;
				layerProjectionView[1].subImage.imageRect.offset.x = 0;
				layerProjectionView[1].subImage.imageRect.offset.y = 0;
				layerProjectionView[1].subImage.imageRect.extent.width = _swapChain->GetSwapChainSize().x;
				layerProjectionView[1].subImage.imageRect.extent.height = _swapChain->GetSwapChainSize().y;
				layerProjectionView[1].subImage.imageArrayIndex = 1;

				XrCompositionLayerProjection layerProjection;
				layerProjection.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
				layerProjection.next = nullptr;
				layerProjection.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
				layerProjection.space = _internals->trackingSpace;
				layerProjection.viewCount = 2;
				layerProjection.views = layerProjectionView;

				std::vector<XrCompositionLayerBaseHeader*> layers;
				layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layerProjection));

				XrFrameEndInfo frameEndInfo;
				frameEndInfo.type = XR_TYPE_FRAME_END_INFO;
				frameEndInfo.next = nullptr;
				frameEndInfo.displayTime = _internals->predictedDisplayTime;
				frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
				frameEndInfo.layerCount = layers.size();
				frameEndInfo.layers = layers.data();
				xrEndFrame(_internals->session, &frameEndInfo);
			}
		};

		if(_internals->RequestDisplayRefreshRateFB)
		{
			_internals->RequestDisplayRefreshRateFB(_internals->session, _preferredFrameRate);
		}

		if(_internals->PerfSettingsSetPerformanceLevelEXT)
		{
			_internals->PerfSettingsSetPerformanceLevelEXT(_internals->session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, (XrPerfSettingsLevelEXT)_minCPULevel);
			_internals->PerfSettingsSetPerformanceLevelEXT(_internals->session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, (XrPerfSettingsLevelEXT)_minGPULevel);
		}

#if XR_USE_PLATFORM_ANDROID
		if(_internals->SetAndroidApplicationThreadKHR)
		{
			_internals->SetAndroidApplicationThreadKHR(_internals->session, XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, _mainThreadID);
		}
#endif

		if(_swapChain && _fixedFoveatedRenderingLevel > 0)
		{
			_swapChain->SetFixedFoveatedRenderingLevel(_fixedFoveatedRenderingLevel, _fixedFoveatedRenderingDynamic);
		}

		XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
		attachInfo.countActionSets = 1;
		attachInfo.actionSets = &_internals->gameActionSet;
		if(!XR_SUCCEEDED(xrAttachSessionActionSets(_internals->session, &attachInfo)))
		{
			RN_ASSERT(false, "failed attaching action sets");
		}

		XrActionSpaceCreateInfo handLeftAimPoseSpaceCreateInfo;
		handLeftAimPoseSpaceCreateInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
		handLeftAimPoseSpaceCreateInfo.next = nullptr;
		handLeftAimPoseSpaceCreateInfo.action = _internals->handLeftAimPoseAction;
		handLeftAimPoseSpaceCreateInfo.subactionPath = XR_NULL_PATH;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.orientation.x = 0.0f;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.orientation.y = 0.0f;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.orientation.z = 0.0f;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.orientation.w = 1.0f;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.position.x = 0.0f;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.position.y = 0.0f;
		handLeftAimPoseSpaceCreateInfo.poseInActionSpace.position.z = 0.0f;
		xrCreateActionSpace(_internals->session, &handLeftAimPoseSpaceCreateInfo, &_internals->handLeftAimPoseSpace);

		XrActionSpaceCreateInfo handLeftGripPoseSpaceCreateInfo;
		handLeftGripPoseSpaceCreateInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
		handLeftGripPoseSpaceCreateInfo.next = nullptr;
		handLeftGripPoseSpaceCreateInfo.action = _internals->handLeftGripPoseAction;
		handLeftGripPoseSpaceCreateInfo.subactionPath = XR_NULL_PATH;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.orientation.x = 0.0f;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.orientation.y = 0.0f;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.orientation.z = 0.0f;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.orientation.w = 1.0f;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.position.x = 0.0f;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.position.y = 0.0f;
		handLeftGripPoseSpaceCreateInfo.poseInActionSpace.position.z = 0.0f;
		xrCreateActionSpace(_internals->session, &handLeftGripPoseSpaceCreateInfo, &_internals->handLeftGripPoseSpace);

		XrActionSpaceCreateInfo handRightAimPoseSpaceCreateInfo;
		handRightAimPoseSpaceCreateInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
		handRightAimPoseSpaceCreateInfo.next = nullptr;
		handRightAimPoseSpaceCreateInfo.action = _internals->handRightAimPoseAction;
		handRightAimPoseSpaceCreateInfo.subactionPath = XR_NULL_PATH;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.orientation.x = 0.0f;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.orientation.y = 0.0f;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.orientation.z = 0.0f;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.orientation.w = 1.0f;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.position.x = 0.0f;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.position.y = 0.0f;
		handRightAimPoseSpaceCreateInfo.poseInActionSpace.position.z = 0.0f;
		xrCreateActionSpace(_internals->session, &handRightAimPoseSpaceCreateInfo, &_internals->handRightAimPoseSpace);

		XrActionSpaceCreateInfo handRightGripPoseSpaceCreateInfo;
		handRightGripPoseSpaceCreateInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
		handRightGripPoseSpaceCreateInfo.next = nullptr;
		handRightGripPoseSpaceCreateInfo.action = _internals->handRightGripPoseAction;
		handRightGripPoseSpaceCreateInfo.subactionPath = XR_NULL_PATH;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.orientation.x = 0.0f;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.orientation.y = 0.0f;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.orientation.z = 0.0f;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.orientation.w = 1.0f;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.position.x = 0.0f;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.position.y = 0.0f;
		handRightGripPoseSpaceCreateInfo.poseInActionSpace.position.z = 0.0f;
		xrCreateActionSpace(_internals->session, &handRightGripPoseSpaceCreateInfo, &_internals->handRightGripPoseSpace);
	}

	void OpenXRWindow::StopRendering()
	{
		if(_internals->session != XR_NULL_HANDLE)
		{
			xrDestroySession(_internals->session);
		}

		delete[] _internals->views;

		if(_swapChain)
		{
#ifdef XR_USE_GRAPHICS_API_D3D12
			if(_swapChainType == SwapChainType::D3D12)
			{
				OpenXRD3D12SwapChain *swapChain = static_cast<OpenXRD3D12SwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
			}
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
			if(_swapChainType == SwapChainType::Vulkan)
			{
				OpenXRVulkanSwapChain *swapChain = static_cast<OpenXRVulkanSwapChain*>(_swapChain);
				swapChain->Release();
				_swapChain = nullptr;
				return;
			}
#endif
		}
	}

	bool OpenXRWindow::IsRendering() const
	{
		return true;
	}

	void OpenXRWindow::SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic)
	{
		_fixedFoveatedRenderingLevel = level;
		_fixedFoveatedRenderingDynamic = dynamic;

		if(_swapChain)
		{
			_swapChain->SetFixedFoveatedRenderingLevel(level, dynamic);
		}
	}

	void OpenXRWindow::SetPreferredFramerate(float framerate)
	{
		_preferredFrameRate = framerate;

		if(_internals->session != XR_NULL_HANDLE && _internals->RequestDisplayRefreshRateFB)
		{
			_internals->RequestDisplayRefreshRateFB(_internals->session, framerate);
		}
	}

	void OpenXRWindow::SetPerformanceLevel(uint8 cpuLevel, uint8 gpuLevel)
	{
		_minCPULevel = cpuLevel * 25;
		_minGPULevel = gpuLevel * 25;

		if(_internals->session != XR_NULL_HANDLE && _internals->PerfSettingsSetPerformanceLevelEXT)
		{
			_internals->PerfSettingsSetPerformanceLevelEXT(_internals->session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, (XrPerfSettingsLevelEXT)_minCPULevel);
			_internals->PerfSettingsSetPerformanceLevelEXT(_internals->session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, (XrPerfSettingsLevelEXT)_minGPULevel);
		}
	}

	Vector2 OpenXRWindow::GetSize() const
	{
		return _swapChain->GetSwapChainSize();
	}

	Framebuffer *OpenXRWindow::GetFramebuffer() const
	{
		return _swapChain->GetSwapChainFramebuffer();
	}

	Framebuffer *OpenXRWindow::GetFramebuffer(uint8 eye) const
	{
		RN_ASSERT(eye < 2, "Eye Index needs to be 0 or 1");
		return _swapChain->GetSwapChainFramebuffer();
	}

	static Matrix GetProjectionMatrixForXRFovf(const XrFovf &fov, float near, float far)
	{
		float tan_left = tanf(fov.angleLeft);
		float tan_right = tanf(fov.angleRight);

		float tan_down = tanf(fov.angleDown);
		float tan_up = tanf(fov.angleUp);

		float tan_width = tan_right - tan_left;
		float tan_height = tan_up - tan_down;

		float a11 = 2.0f / tan_width;
		float a22 = 2.0f / tan_height;

		float a31 = (tan_right + tan_left) / tan_width;
		float a32 = (tan_up + tan_down) / tan_height;
		float a33 = far / (far - near) - 1.0f;

		float a43 = (far * near) / (far - near);

		Matrix result;
		result.m[0] = a11;
		result.m[1] = 0;
		result.m[2] = 0;
		result.m[3] = 0;
		result.m[4] = 0;
		result.m[5] = a22;
		result.m[6] = 0;
		result.m[7] = 0;
		result.m[8] = a31;
		result.m[9] = a32;
		result.m[10] = a33;
		result.m[11] = -1;
		result.m[12] = 0;
		result.m[13] = 0;
		result.m[14] = a43;
		result.m[15] = 0;

		return result;
	}

	void OpenXRWindow::BeginFrame(float delta)
	{
		if(_internals->session == XR_NULL_HANDLE || !_isSessionRunning) return;

		_actualFrameIndex++;

		XrFrameWaitInfo frameWaitInfo;
		frameWaitInfo.type = XR_TYPE_FRAME_WAIT_INFO;
		frameWaitInfo.next = nullptr;
		XrFrameState frameState;
		frameState.type = XR_TYPE_FRAME_STATE;
		frameState.next = nullptr;
		xrWaitFrame(_internals->session, &frameWaitInfo, &frameState);

		_internals->predictedDisplayTime = frameState.predictedDisplayTime;

		XrFrameBeginInfo frameBeginInfo;
		frameBeginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;
		frameBeginInfo.next = nullptr;
		xrBeginFrame(_internals->session, &frameBeginInfo);
	}

	void OpenXRWindow::Update(float delta, float near, float far)
	{
		_hmdTrackingState.mode = VRHMDTrackingState::Mode::Paused;

		while(1)
		{
			XrEventDataBuffer event;
			event.type = XR_TYPE_EVENT_DATA_BUFFER;
			event.next = nullptr;
			XrResult result = xrPollEvent(_internals->instance, &event);
			if(result == XR_SUCCESS)
			{
				switch (event.type)
				{
					case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
					{
						const XrEventDataSessionStateChanged &sessionStateChangedEvent =
								*reinterpret_cast<XrEventDataSessionStateChanged *>(&event);
						if(sessionStateChangedEvent.state == XR_SESSION_STATE_READY)
						{
							RNDebug("Session State: Ready");
							XrSessionBeginInfo beginInfo;
							beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
							beginInfo.next = nullptr;
							beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
							xrBeginSession(_internals->session, &beginInfo);
							_isSessionRunning = true;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_STOPPING)
						{
							RNDebug("Session State: Stopping");
							_hasSynchronization = false;
							_isSessionRunning = false;
							xrEndSession(_internals->session);
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_EXITING)
						{
							RNDebug("Session State: Exiting");
							xrDestroySession(_internals->session);
							_internals->session = XR_NULL_HANDLE;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_SYNCHRONIZED)
						{
							RNDebug("Session State: Synchronized");
							_hasSynchronization = true;
							_hasVisibility = false;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_VISIBLE)
						{
							RNDebug("Session State: Visible");
							_hasVisibility = true;
							_hasInputFocus = false;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_FOCUSED)
						{
							RNDebug("Session State: Focused");
							_hasInputFocus = true;
						}
						break;
					}
					case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
					{
						const XrEventDataInstanceLossPending &instance_loss_pending_event =
								*reinterpret_cast<XrEventDataInstanceLossPending *>(&event);
						// ...
						break;
					}
					case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
					{
						const XrEventDataReferenceSpaceChangePending &referenceSpaceChangePendingEvent =
								*reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&event);
						RNDebug("Changed pose: (" << referenceSpaceChangePendingEvent.poseInPreviousSpace.position.x << ", " << referenceSpaceChangePendingEvent.poseInPreviousSpace.position.y << ", " << referenceSpaceChangePendingEvent.poseInPreviousSpace.position.z << ")");
						NotificationManager::GetSharedInstance()->PostNotification(kRNVRDidRecenter, nullptr);
						break;
					}
				}
			}
			else
			{
				break;
			}
		}

		if(_internals->session == XR_NULL_HANDLE || !_isSessionRunning) return;

		XrViewLocateInfo locateInfo;
		locateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
		locateInfo.next = nullptr;
		locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		locateInfo.displayTime = _internals->predictedDisplayTime;
		locateInfo.space = _internals->trackingSpace;

		XrViewState viewState;
		viewState.type = XR_TYPE_VIEW_STATE;
		viewState.next = nullptr;
		viewState.viewStateFlags = XR_VIEW_STATE_ORIENTATION_VALID_BIT | XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_TRACKED_BIT | XR_VIEW_STATE_POSITION_TRACKED_BIT;

		uint32_t viewCount = 2;
		xrLocateViews(_internals->session, &locateInfo, &viewState, viewCount, &viewCount, _internals->views);

		RN::Vector3 leftEyePosition = Vector3(_internals->views[0].pose.position.x, _internals->views[0].pose.position.y, _internals->views[0].pose.position.z);
		RN::Vector3 rightEyePosition = Vector3(_internals->views[1].pose.position.x, _internals->views[1].pose.position.y, _internals->views[1].pose.position.z);
		RN::Quaternion leftEyeRotation = Quaternion(_internals->views[0].pose.orientation.x, _internals->views[0].pose.orientation.y, _internals->views[0].pose.orientation.z, _internals->views[0].pose.orientation.w);
		RN::Quaternion rightEyeRotation = Quaternion(_internals->views[1].pose.orientation.x, _internals->views[1].pose.orientation.y, _internals->views[1].pose.orientation.z, _internals->views[1].pose.orientation.w);
		_hmdTrackingState.position = (leftEyePosition + rightEyePosition) * 0.5f;
		_hmdTrackingState.rotation = leftEyeRotation.GetLerpSpherical(rightEyeRotation, 0.5f);

		_hmdTrackingState.eyeOffset[0] = _hmdTrackingState.rotation.GetConjugated().GetRotatedVector(leftEyePosition - _hmdTrackingState.position);
		_hmdTrackingState.eyeOffset[1] = _hmdTrackingState.rotation.GetConjugated().GetRotatedVector(rightEyePosition - _hmdTrackingState.position);
		_hmdTrackingState.eyeProjection[0] = GetProjectionMatrixForXRFovf(_internals->views[0].fov, near, far);
		_hmdTrackingState.eyeProjection[1] = GetProjectionMatrixForXRFovf(_internals->views[1].fov, near, far);

		if(_hasVisibility && _hasInputFocus)
		{
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Rendering;
		}

		//This is fine on android too, but is spamming too much into the logs on quest
#if !RN_PLATFORM_ANDROID
		XrPath leftHandUserPath;
		xrStringToPath(_internals->instance, "/user/hand/left", &leftHandUserPath);
		XrInteractionProfileState leftHandInteractionProfileState{XR_TYPE_INTERACTION_PROFILE_STATE};
		xrGetCurrentInteractionProfile(_internals->session, leftHandUserPath, &leftHandInteractionProfileState);

		XrPath rightHandUserPath;
		xrStringToPath(_internals->instance, "/user/hand/right", &rightHandUserPath);
		XrInteractionProfileState rightHandInteractionProfileState{ XR_TYPE_INTERACTION_PROFILE_STATE };
		xrGetCurrentInteractionProfile(_internals->session, rightHandUserPath, &rightHandInteractionProfileState);

		_controllerTrackingState[0].type = GetControllerTypeForInteractionProfile(_internals->instance, leftHandInteractionProfileState.interactionProfile);
		_controllerTrackingState[1].type = GetControllerTypeForInteractionProfile(_internals->instance, rightHandInteractionProfileState.interactionProfile);
#else
		_controllerTrackingState[0].type = VRControllerTrackingState::Type::OculusTouchController;
		_controllerTrackingState[1].type = VRControllerTrackingState::Type::OculusTouchController;
#endif

		_controllerTrackingState[0].hasHaptics = true;
		_controllerTrackingState[0].active = false;
		_controllerTrackingState[0].tracking = false;
		_controllerTrackingState[0].hapticsSampleLength = 0.0;
		_controllerTrackingState[0].hapticsMaxSamples = 0;
		_controllerTrackingState[1].active = false;
		_controllerTrackingState[1].tracking = false;
		_controllerTrackingState[1].hasHaptics = true;
		_controllerTrackingState[1].hapticsSampleLength = 0.0;
		_controllerTrackingState[1].hapticsMaxSamples = 0;

		_handTrackingState[0].pinchStrength[0] = 0.0f;
		_handTrackingState[0].pinchStrength[1] = 0.0f;
		_handTrackingState[0].pinchStrength[2] = 0.0f;
		_handTrackingState[0].pinchStrength[3] = 0.0f;
		_handTrackingState[0].active = false;
		_handTrackingState[0].tracking = false;
		_handTrackingState[1].pinchStrength[0] = 0.0f;
		_handTrackingState[1].pinchStrength[1] = 0.0f;
		_handTrackingState[1].pinchStrength[2] = 0.0f;
		_handTrackingState[1].pinchStrength[3] = 0.0f;
		_handTrackingState[1].active = false;
		_handTrackingState[1].tracking = false;

		if(!_hasInputFocus) return;

		XrActiveActionSet activeActionSet{_internals->gameActionSet, XR_NULL_PATH};
		XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
		syncInfo.countActiveActionSets = 1;
		syncInfo.activeActionSets = &activeActionSet;
		xrSyncActions(_internals->session, &syncInfo);

		//Left hand
		XrActionStatePose handLeftState{XR_TYPE_ACTION_STATE_POSE};
		XrActionStateGetInfo getHandLeftInfo{XR_TYPE_ACTION_STATE_GET_INFO};

		getHandLeftInfo.action = _internals->handLeftAimPoseAction;
		xrGetActionStatePose(_internals->session, &getHandLeftInfo, &handLeftState);

		getHandLeftInfo.action = _internals->handLeftGripPoseAction;
		xrGetActionStatePose(_internals->session, &getHandLeftInfo, &handLeftState);

		_controllerTrackingState[0].active = handLeftState.isActive;
		_controllerTrackingState[0].tracking = handLeftState.isActive;
		if(handLeftState.isActive)
		{
			XrSpaceLocation aimLocation {XR_TYPE_SPACE_LOCATION};
			xrLocateSpace(_internals->handLeftAimPoseSpace, _internals->trackingSpace, _internals->predictedDisplayTime, &aimLocation);

			if(aimLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[0].positionAim = Vector3(aimLocation.pose.position.x, aimLocation.pose.position.y, aimLocation.pose.position.z);
			}
			if(aimLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[0].rotationAim = Quaternion(aimLocation.pose.orientation.x, aimLocation.pose.orientation.y, aimLocation.pose.orientation.z, aimLocation.pose.orientation.w);
			}

			XrSpaceVelocity velocity {XR_TYPE_SPACE_VELOCITY};
			XrSpaceLocation gripLocation {XR_TYPE_SPACE_LOCATION, &velocity};
			xrLocateSpace(_internals->handLeftGripPoseSpace, _internals->trackingSpace, _internals->predictedDisplayTime, &gripLocation);

			if(gripLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[0].positionGrip = Vector3(gripLocation.pose.position.x, gripLocation.pose.position.y, gripLocation.pose.position.z);
			}
			if(gripLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[0].rotationGrip = Quaternion(gripLocation.pose.orientation.x, gripLocation.pose.orientation.y, gripLocation.pose.orientation.z, gripLocation.pose.orientation.w);
			}

			if(velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
			{
				_controllerTrackingState[0].velocityLinear = Vector3(velocity.linearVelocity.x, velocity.linearVelocity.y, velocity.linearVelocity.z);
			}
			if(velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
			{
				_controllerTrackingState[0].velocityAngular = Vector3(velocity.angularVelocity.x, velocity.angularVelocity.y, velocity.angularVelocity.z);
			}

			XrActionStateFloat handTriggerState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handTriggerGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handTriggerGetInfo.action = _internals->handLeftTriggerAction;
			xrGetActionStateFloat(_internals->session, &handTriggerGetInfo, &handTriggerState);
			_controllerTrackingState[0].indexTrigger = handTriggerState.currentState;

			XrActionStateFloat handGrabState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handGrabGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handGrabGetInfo.action = _internals->handLeftGrabAction;
			xrGetActionStateFloat(_internals->session, &handGrabGetInfo, &handGrabState);
			_controllerTrackingState[0].handTrigger = handGrabState.currentState;

			XrActionStateFloat handThumbstickXState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handThumbstickXGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handThumbstickXGetInfo.action = _internals->handLeftThumbstickXAction;
			xrGetActionStateFloat(_internals->session, &handThumbstickXGetInfo, &handThumbstickXState);
			_controllerTrackingState[0].thumbstick.x = handThumbstickXState.currentState;

			XrActionStateFloat handThumbstickYState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handThumbstickYGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handThumbstickYGetInfo.action = _internals->handLeftThumbstickYAction;
			xrGetActionStateFloat(_internals->session, &handThumbstickYGetInfo, &handThumbstickYState);
			_controllerTrackingState[0].thumbstick.y = handThumbstickYState.currentState;

			XrActionStateBoolean handThumbstickPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handThumbstickPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handThumbstickPressGetInfo.action = _internals->handLeftThumbstickPressAction;
			xrGetActionStateBoolean(_internals->session, &handThumbstickPressGetInfo, &handThumbstickPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::Stick] = handThumbstickPressState.currentState;

			XrActionStateFloat handTrackpadXState{ XR_TYPE_ACTION_STATE_FLOAT };
			XrActionStateGetInfo handTrackpadXGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadXGetInfo.action = _internals->handLeftTrackpadXAction;
			xrGetActionStateFloat(_internals->session, &handTrackpadXGetInfo, &handTrackpadXState);
			_controllerTrackingState[0].trackpad.x = handTrackpadXState.currentState;

			XrActionStateFloat handTrackpadYState{ XR_TYPE_ACTION_STATE_FLOAT };
			XrActionStateGetInfo handTrackpadYGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadYGetInfo.action = _internals->handLeftTrackpadYAction;
			xrGetActionStateFloat(_internals->session, &handTrackpadYGetInfo, &handTrackpadYState);
			_controllerTrackingState[0].trackpad.y = handTrackpadYState.currentState;

			XrActionStateBoolean handTrackpadTouchState{ XR_TYPE_ACTION_STATE_BOOLEAN };
			XrActionStateGetInfo handTrackpadTouchGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadTouchGetInfo.action = _internals->handLeftTrackpadTouchAction;
			xrGetActionStateBoolean(_internals->session, &handTrackpadTouchGetInfo, &handTrackpadTouchState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::PadTouched] = handTrackpadTouchState.currentState;

			XrActionStateBoolean handTrackpadPressState{ XR_TYPE_ACTION_STATE_BOOLEAN };
			XrActionStateGetInfo handTrackpadPressGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadPressGetInfo.action = _internals->handLeftTrackpadPressAction;
			xrGetActionStateBoolean(_internals->session, &handTrackpadPressGetInfo, &handTrackpadPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::Pad] = handTrackpadPressState.currentState;

			XrActionStateBoolean handButtonUpperPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonUpperPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonUpperPressGetInfo.action = _internals->handLeftButtonUpperPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonUpperPressGetInfo, &handButtonUpperPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::BY] = handButtonUpperPressState.currentState;

			XrActionStateBoolean handButtonSystemPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonSystemPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonSystemPressGetInfo.action = _internals->handLeftButtonSystemPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonSystemPressGetInfo, &handButtonSystemPressState);
			//This is needed because the pressing the Y button will trigger both, the upper button action and the system button action on quest
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::Start] = !_controllerTrackingState[0].button[VRControllerTrackingState::Button::BY] && handButtonSystemPressState.currentState;

			XrActionStateBoolean handButtonLowerPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonLowerPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonLowerPressGetInfo.action = _internals->handLeftButtonLowerPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonLowerPressGetInfo, &handButtonLowerPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::AX] = handButtonLowerPressState.currentState;

			if(_currentHapticsIndex[0] < _haptics[0].sampleCount)
			{
				float strength = _haptics[0].samples[_currentHapticsIndex[0]++];
				XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
				hapticActionInfo.action = _internals->handLeftHapticsAction;
				hapticActionInfo.subactionPath = XR_NULL_PATH;
				XrHapticVibration hapticVibration{XR_TYPE_HAPTIC_VIBRATION};
				hapticVibration.duration = delta * 1000000000.0; //nanoseconds
				hapticVibration.frequency = XR_FREQUENCY_UNSPECIFIED;
				hapticVibration.amplitude = strength;
				xrApplyHapticFeedback(_internals->session, &hapticActionInfo, (XrHapticBaseHeader*)&hapticVibration);
				_hapticsStopped[0] = false;
			}
		}

		//Right hand
		XrActionStatePose handRightState{XR_TYPE_ACTION_STATE_POSE};
		XrActionStateGetInfo getHandRightInfo{XR_TYPE_ACTION_STATE_GET_INFO};

		getHandRightInfo.action = _internals->handRightAimPoseAction;
		xrGetActionStatePose(_internals->session, &getHandRightInfo, &handRightState);

		getHandRightInfo.action = _internals->handRightGripPoseAction;
		xrGetActionStatePose(_internals->session, &getHandRightInfo, &handRightState);

		_controllerTrackingState[1].active = handRightState.isActive;
		_controllerTrackingState[1].tracking = handRightState.isActive;
		if(handRightState.isActive)
		{
			XrSpaceLocation aimLocation {XR_TYPE_SPACE_LOCATION};
			xrLocateSpace(_internals->handRightAimPoseSpace, _internals->trackingSpace, _internals->predictedDisplayTime, &aimLocation);

			if(aimLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[1].positionAim = Vector3(aimLocation.pose.position.x, aimLocation.pose.position.y, aimLocation.pose.position.z);
			}
			if(aimLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[1].rotationAim = Quaternion(aimLocation.pose.orientation.x, aimLocation.pose.orientation.y, aimLocation.pose.orientation.z, aimLocation.pose.orientation.w);
			}

			XrSpaceVelocity velocity {XR_TYPE_SPACE_VELOCITY};
			XrSpaceLocation gripLocation {XR_TYPE_SPACE_LOCATION, &velocity};
			xrLocateSpace(_internals->handRightGripPoseSpace, _internals->trackingSpace, _internals->predictedDisplayTime, &gripLocation);

			if(gripLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[1].positionGrip = Vector3(gripLocation.pose.position.x, gripLocation.pose.position.y, gripLocation.pose.position.z);
			}
			if(gripLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[1].rotationGrip = Quaternion(gripLocation.pose.orientation.x, gripLocation.pose.orientation.y, gripLocation.pose.orientation.z, gripLocation.pose.orientation.w);
			}

			if(velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
			{
				_controllerTrackingState[1].velocityLinear = Vector3(velocity.linearVelocity.x, velocity.linearVelocity.y, velocity.linearVelocity.z);
			}
			if(velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
			{
				_controllerTrackingState[1].velocityAngular = Vector3(velocity.angularVelocity.x, velocity.angularVelocity.y, velocity.angularVelocity.z);
			}

			XrActionStateFloat handTriggerState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handTriggerGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handTriggerGetInfo.action = _internals->handRightTriggerAction;
			xrGetActionStateFloat(_internals->session, &handTriggerGetInfo, &handTriggerState);
			_controllerTrackingState[1].indexTrigger = handTriggerState.currentState;

			XrActionStateFloat handGrabState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handGrabGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handGrabGetInfo.action = _internals->handRightGrabAction;
			xrGetActionStateFloat(_internals->session, &handGrabGetInfo, &handGrabState);
			_controllerTrackingState[1].handTrigger = handGrabState.currentState;

			XrActionStateFloat handThumbstickXState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handThumbstickXGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handThumbstickXGetInfo.action = _internals->handRightThumbstickXAction;
			xrGetActionStateFloat(_internals->session, &handThumbstickXGetInfo, &handThumbstickXState);
			_controllerTrackingState[1].thumbstick.x = handThumbstickXState.currentState;

			XrActionStateFloat handThumbstickYState{XR_TYPE_ACTION_STATE_FLOAT};
			XrActionStateGetInfo handThumbstickYGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handThumbstickYGetInfo.action = _internals->handRightThumbstickYAction;
			xrGetActionStateFloat(_internals->session, &handThumbstickYGetInfo, &handThumbstickYState);
			_controllerTrackingState[1].thumbstick.y = handThumbstickYState.currentState;

			XrActionStateBoolean handThumbstickPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handThumbstickPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handThumbstickPressGetInfo.action = _internals->handRightThumbstickPressAction;
			xrGetActionStateBoolean(_internals->session, &handThumbstickPressGetInfo, &handThumbstickPressState);
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::Stick] = handThumbstickPressState.currentState;

			XrActionStateFloat handTrackpadXState{ XR_TYPE_ACTION_STATE_FLOAT };
			XrActionStateGetInfo handTrackpadXGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadXGetInfo.action = _internals->handRightTrackpadXAction;
			xrGetActionStateFloat(_internals->session, &handTrackpadXGetInfo, &handTrackpadXState);
			_controllerTrackingState[1].trackpad.x = handTrackpadXState.currentState;

			XrActionStateFloat handTrackpadYState{ XR_TYPE_ACTION_STATE_FLOAT };
			XrActionStateGetInfo handTrackpadYGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadYGetInfo.action = _internals->handRightTrackpadYAction;
			xrGetActionStateFloat(_internals->session, &handTrackpadYGetInfo, &handTrackpadYState);
			_controllerTrackingState[1].trackpad.y = handTrackpadYState.currentState;

			XrActionStateBoolean handTrackpadTouchState{ XR_TYPE_ACTION_STATE_BOOLEAN };
			XrActionStateGetInfo handTrackpadTouchGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadTouchGetInfo.action = _internals->handRightTrackpadTouchAction;
			xrGetActionStateBoolean(_internals->session, &handTrackpadTouchGetInfo, &handTrackpadTouchState);
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::PadTouched] = handTrackpadTouchState.currentState;

			XrActionStateBoolean handTrackpadPressState{ XR_TYPE_ACTION_STATE_BOOLEAN };
			XrActionStateGetInfo handTrackpadPressGetInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			handTrackpadPressGetInfo.action = _internals->handRightTrackpadPressAction;
			xrGetActionStateBoolean(_internals->session, &handTrackpadPressGetInfo, &handTrackpadPressState);
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::Pad] = handTrackpadPressState.currentState;

			XrActionStateBoolean handButtonSystemPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonSystemPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonSystemPressGetInfo.action = _internals->handRightButtonSystemPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonSystemPressGetInfo, &handButtonSystemPressState);
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::Start] = handButtonSystemPressState.currentState;

			XrActionStateBoolean handButtonUpperPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonUpperPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonUpperPressGetInfo.action = _internals->handRightButtonUpperPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonUpperPressGetInfo, &handButtonUpperPressState);
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::BY] = handButtonUpperPressState.currentState;

			XrActionStateBoolean handButtonLowerPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonLowerPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonLowerPressGetInfo.action = _internals->handRightButtonLowerPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonLowerPressGetInfo, &handButtonLowerPressState);
			_controllerTrackingState[1].button[VRControllerTrackingState::Button::AX] = handButtonLowerPressState.currentState;

			if(_currentHapticsIndex[1] < _haptics[1].sampleCount)
			{
				float strength = _haptics[1].samples[_currentHapticsIndex[1]++];
				XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
				hapticActionInfo.action = _internals->handRightHapticsAction;
				hapticActionInfo.subactionPath = XR_NULL_PATH;
				XrHapticVibration hapticVibration{XR_TYPE_HAPTIC_VIBRATION};
				hapticVibration.duration = delta * 1000000000.0; //nanoseconds
				hapticVibration.frequency = XR_FREQUENCY_UNSPECIFIED;
				hapticVibration.amplitude = strength;
				xrApplyHapticFeedback(_internals->session, &hapticActionInfo, (XrHapticBaseHeader*)&hapticVibration);
				_hapticsStopped[1] = false;
			}
		}

/*		ovrInputCapabilityHeader capsHeader;
		int i = 0;
		while(vrapi_EnumerateInputDevices(static_cast<ovrMobile*>(_session), i, &capsHeader) >= 0)
		{
			i += 1;
			if(capsHeader.Type == ovrControllerType_Hand)
			{
				ovrInputHandCapabilities handCaps;
				handCaps.Header = capsHeader;
				if(vrapi_GetInputDeviceCapabilities(static_cast<ovrMobile*>(_session), &handCaps.Header) >= 0)
				{
					int handIndex = (handCaps.HandCapabilities & ovrHandCaps_RightHand)?1:0;

					_handTrackingState[handIndex].active = true;

					ovrHandPose handPose;
					handPose.Header.Version = ovrHandVersion_1;
					if(vrapi_GetHandPose(static_cast<ovrMobile*>(_session), handCaps.Header.DeviceID, _predictedDisplayTime, &handPose.Header) >= 0)
					{
						_handTrackingState[handIndex].position = GetVectorForOVRVector(handPose.RootPose.Position);
						_handTrackingState[handIndex].rotation = GetQuaternionForOVRQuaternion(handPose.RootPose.Orientation);
						_handTrackingState[handIndex].tracking = (handPose.Status == ovrHandTrackingStatus_Tracked);
						_handTrackingState[handIndex].confidence = handPose.HandConfidence == ovrConfidence_HIGH? 255 : 127;
					}

					ovrInputStateHand trackingState;
					trackingState.Header.ControllerType = ovrControllerType_Hand;
					if(vrapi_GetCurrentInputState(static_cast<ovrMobile*>(_session), handCaps.Header.DeviceID, &trackingState.Header) >= 0)
					{
						_handTrackingState[handIndex].pinchStrength[0] = trackingState.PinchStrength[0];
						_handTrackingState[handIndex].pinchStrength[1] = trackingState.PinchStrength[1];
						_handTrackingState[handIndex].pinchStrength[2] = trackingState.PinchStrength[2];
						_handTrackingState[handIndex].pinchStrength[3] = trackingState.PinchStrength[3];

						_handTrackingState[handIndex].menuButton = trackingState.InputStateStatus & ovrInputStateHandStatus_MenuPressed;
					}
				}
			}
		}
		*/
	}

	const String *OpenXRWindow::GetHMDInfoDescription() const
	{
		String *description = new String("Using HMD: ");
		description->Append(RNSTR(_internals->systemProperties.systemName));
		return description->Autorelease();
	}

	const VRHMDTrackingState &OpenXRWindow::GetHMDTrackingState() const
	{
		return _hmdTrackingState;
	}

	const VRControllerTrackingState &OpenXRWindow::GetControllerTrackingState(uint8 index) const
	{
		return _controllerTrackingState[index];
	}

	const VRControllerTrackingState &OpenXRWindow::GetTrackerTrackingState(uint8 index) const
	{
		return _trackerTrackingState;
	}

	const VRHandTrackingState &OpenXRWindow::GetHandTrackingState(uint8 index) const
	{
		return _handTrackingState[index];
	}

	void OpenXRWindow::SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics)
	{
		if(_internals->session == XR_NULL_HANDLE) return;
		if(!_controllerTrackingState[index].hasHaptics) return;

		_currentHapticsIndex[index] = 0;
		_haptics[index] = haptics;
	}

	const String *OpenXRWindow::GetPreferredAudioOutputDeviceID() const
	{
		return nullptr;
	}

	const String *OpenXRWindow::GetPreferredAudioInputDeviceID() const
	{
		return nullptr;
	}

	RenderingDevice *OpenXRWindow::GetOutputDevice(RendererDescriptor *descriptor) const
	{
		return nullptr;
	}

	const Window::SwapChainDescriptor &OpenXRWindow::GetSwapChainDescriptor() const
	{
		return _swapChain->GetSwapChainDescriptor();
	}

	Array *OpenXRWindow::GetRequiredVulkanInstanceExtensions() const
	{
#if XR_USE_GRAPHICS_API_VULKAN
		char names[4096];
		uint32_t size = sizeof(names);
		if(_internals->GetVulkanInstanceExtensionsKHR(_internals->instance, _internals->systemID, size, &size, names) != XR_SUCCESS)
		{
			return nullptr;
		}

		String *extensionString = RNSTR(names);
		RNDebug("Needs vulkan instance extensions: " << extensionString);
		return extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
#else
		return nullptr;
#endif
	}

	Array *OpenXRWindow::GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *device) const
	{
#if XR_USE_GRAPHICS_API_VULKAN
		char names[4096];
		uint32_t size = sizeof(names);
		if(_internals->GetVulkanDeviceExtensionsKHR(_internals->instance, _internals->systemID, size, &size, names) != XR_SUCCESS)
		{
			return nullptr;
		}

		String *extensionString = RNSTR(names);
		RNDebug("Needs vulkan device extensions: " << extensionString);
		return extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
#else
		return nullptr;
#endif
	}

	VRWindow::DeviceType OpenXRWindow::GetDeviceType() const
	{
		return _deviceType;
	}
}

