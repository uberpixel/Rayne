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

#include <dlfcn.h>
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

		XrPath picoNeo3Controller;
		xrStringToPath(instance, "/interaction_profiles/bytedance/pico_neo3_controller", &picoNeo3Controller);

		XrPath pico4Controller;
		xrStringToPath(instance, "/interaction_profiles/bytedance/pico4_controller", &pico4Controller);

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
		else if(interactionProfile == picoNeo3Controller)
		{
			return VRControllerTrackingState::Type::PicoNeo3Controller;
		}
		else if(interactionProfile == pico4Controller)
		{
			return VRControllerTrackingState::Type::PicoNeo3Controller;
		}

		return VRControllerTrackingState::Type::None;
	}

	OpenXRWindow::OpenXRWindow() : _internals(new OpenXRWindowInternals()), _runtimeName(nullptr), _actualFrameIndex(0), _predictedDisplayTime(0.0), _currentHapticsIndex{0, 0}, _hapticsStopped{true, true}, _preferredFrameRate(0.0f), _minCPULevel(XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT), _minGPULevel(XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT), _fixedFoveatedRenderingLevel(2), _fixedFoveatedRenderingDynamic(false), _isLocalDimmingEnabled(false), _isSessionRunning(false), _hasSynchronization(false), _hasVisibility(false), _hasInputFocus(false), _mainLayer(nullptr), _layersUnderlay(new Array()), _layersOverlay(new Array())
	{
		_supportsVulkan = false;
		_supportsPreferredFramerate = false;
		_supportsPerformanceLevels = false;
		_supportsAndroidThreadType = false;
		_supportsFoveatedRendering = false;
		_supportsLocalDimming = false;
		_supportsVisibilityMask = false;
		_supportsPassthrough = false;
		_supportsCompositionLayerSettings = false;

#if RN_OPENXR_SUPPORTS_PICO_LOADER
		_internals->_supportsControllerInteractionPICO = false;
#endif

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

		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		jclass build_class = env->FindClass("android/os/Build");
		jfieldID model_id = env->GetStaticFieldID(build_class, "MANUFACTURER", "Ljava/lang/String;");
		jstring model_obj  = (jstring)env->GetStaticObjectField(build_class, model_id);
		const char *manufacturerName = env->GetStringUTFChars(model_obj, 0); //TODO: Pretty sure the string needs to be freed again later
		String *manufacturerNameString = RNSTR(manufacturerName);
		manufacturerNameString->MakeLowercase();
		RNDebug("Android Device manufacturer: " << manufacturerNameString);

		void *module = nullptr;
		xrGetInstanceProcAddr = nullptr;

		//TODO: Ideally these should all use the same official loader library,
		// unfortunately PICOs extensions are not official yet and even oculus needs their own loader on quest
		// So not much I can do here for now, but this can definitely be improved in the future
		//UPDATE: The situation is better now: PICO now doesn't need special extensions,
		// but needs their own loader and META now supports the official loader

		//TODO: The manufacturer names could potentially change in the future. Pico could rebrand,
		// Oculus will most likely become Meta.

#if RN_OPENXR_SUPPORTS_PICO_LOADER
		if(!module && manufacturerNameString->HasPrefix(RNCSTR("pico")))
		{
			module = dlopen("libopenxr_loader_pico.so", RTLD_NOW | RTLD_LOCAL);
		}
#endif
#if RN_OPENXR_SUPPORTS_METAQUEST_LOADER
		if(!module)// && manufacturerNameString->HasPrefix(RNCSTR("oculus")))
		{
			module = dlopen("libopenxr_loader_meta.so", RTLD_NOW | RTLD_LOCAL);
		}
#endif

		if(!module)
		{
			//Fallback to the default loader name if the previous platform specific ones don't exist
			module = dlopen("libopenxr_loader.so", RTLD_NOW | RTLD_LOCAL);
		}

		if(module)
		{
			xrGetInstanceProcAddr = reinterpret_cast<PFN_xrGetInstanceProcAddr>(dlsym(module, "xrGetInstanceProcAddr"));
		}

		if(!module || !xrGetInstanceProcAddr)
		{
			RNError("Couldn't load OpenXR loader");

			if(module)
			{
				dlclose(module);
				module = nullptr;
			}

			//TODO: Handle this somehow...
			RN_ASSERT(false, "No OpenXR Loader found!");
		}

		PopulateOpenXRDispatchTable(XR_NULL_HANDLE);

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
			else if(std::strcmp(extension.extensionName, XR_KHR_VISIBILITY_MASK_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsVisibilityMask = true;
			}
			else if(std::strcmp(extension.extensionName, XR_FB_PASSTHROUGH_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsPassthrough = true;
			}
			else if(std::strcmp(extension.extensionName, XR_FB_COMPOSITION_LAYER_SETTINGS_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsCompositionLayerSettings = true;
			}
#if XR_USE_PLATFORM_ANDROID
			else if(std::strcmp(extension.extensionName, XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsAndroidThreadType = true;
			}
#endif
#if RN_OPENXR_SUPPORTS_PICO_LOADER
				else if(std::strcmp(extension.extensionName, "XR_PICO_controller_interaction") == 0)
			{
				extensions.push_back(extension.extensionName);
				_internals->_supportsControllerInteractionPICO = true;
			}
#elif RN_OPENXR_SUPPORTS_METAQUEST_LOADER
			else if(std::strcmp(extension.extensionName, XR_META_LOCAL_DIMMING_EXTENSION_NAME) == 0)
			{
				extensions.push_back(extension.extensionName);
				_supportsLocalDimming = true;
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

		//TODO: This will only be correct for vulkan and only for the oculus extensions, PICO 4 will claim that it supports the FB FFR extension, but then not actually work with them
		if(numberOfSupportedFoveationExtensions == 4
#if RN_OPENXR_SUPPORTS_PICO_LOADER
			&& !_internals->_supportsControllerInteractionPICO
#endif
				)
		{
			_supportsFoveatedRendering = true;
		}

		XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
		createInfo.next = platformSpecificInstanceCreateInfo;
		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.enabledExtensionNames = extensions.data();

		const RN::String *applicationTitle = Kernel::GetSharedInstance()->GetApplication()->GetTitle();
		if(applicationTitle) strcpy(createInfo.applicationInfo.applicationName, applicationTitle->GetUTF8String());
		else strcpy(createInfo.applicationInfo.applicationName, "NO TITLE");
		createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;

		_internals->instance = XR_NULL_HANDLE;
		XrResult createInstanceResult = xrCreateInstance(&createInfo, &_internals->instance);
		if(createInstanceResult != XR_SUCCESS)
		{
			//TODO: For some reason this fails regularly on Quest
			RN_ASSERT(false, "Failed creating OpenXR instance");
		}

#if XR_USE_PLATFORM_ANDROID
		PopulateOpenXRDispatchTable(_internals->instance); //Fetch remaining methods that require the instance pointer
#endif

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
		else if(std::strcmp(_internals->systemProperties.systemName, "Pico Pico Neo 3") == 0)
		{
			_deviceType = DeviceType::PicoVR;
		}
		else if(std::strcmp(_internals->systemProperties.systemName, "PICO 4") == 0)
		{
			_deviceType = DeviceType::PicoVR;
		}
		else
		{
			_deviceType = DeviceType::Unknown;
		}

		XrInstanceProperties instanceProperties = {};
		instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;
		xrGetInstanceProperties(_internals->instance, &instanceProperties);
		_runtimeName = RNSTR(instanceProperties.runtimeName)->Retain();
		RNInfo("Active OpenXR Runtime: " << _runtimeName);

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
			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction*)(&_internals->EnumerateDisplayRefreshRatesFB))))
			{

			}

			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&_internals->GetDisplayRefreshRateFB))))
			{

			}

			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&_internals->RequestDisplayRefreshRateFB))))
			{

			}
		}

		if(_supportsPerformanceLevels)
		{
			//XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME
			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrPerfSettingsSetPerformanceLevelEXT", (PFN_xrVoidFunction*)(&_internals->PerfSettingsSetPerformanceLevelEXT))))
			{

			}
		}

		if(_supportsFoveatedRendering)
		{
			//XR_FB_FOVEATION_EXTENSION_NAME
			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrCreateFoveationProfileFB", (PFN_xrVoidFunction*)(&_internals->CreateFoveationProfileFB))))
			{

			}

			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrDestroyFoveationProfileFB", (PFN_xrVoidFunction*)(&_internals->DestroyFoveationProfileFB))))
			{

			}

			//XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME
			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrUpdateSwapchainFB", (PFN_xrVoidFunction*)(&_internals->UpdateSwapchainFB))))
			{

			}

			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrGetSwapchainStateFB", (PFN_xrVoidFunction*)(&_internals->GetSwapchainStateFB))))
			{

			}
		}

		if(_supportsVisibilityMask)
		{
			//XR_KHR_VISIBILITY_MASK_EXTENSION_NAME
			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrGetVisibilityMaskKHR", (PFN_xrVoidFunction*)(&_internals->GetVisibilityMaskKHR))))
			{

			}
		}

#if XR_USE_PLATFORM_ANDROID
		if(_supportsAndroidThreadType)
		{
			//XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME
			if(XR_FAILED(xrGetInstanceProcAddr(_internals->instance, "xrSetAndroidApplicationThreadKHR", (PFN_xrVoidFunction*)(&_internals->SetAndroidApplicationThreadKHR))))
			{

			}
		}
#endif
	}

	OpenXRWindow::~OpenXRWindow()
	{
		SafeRelease(_layersUnderlay);
		SafeRelease(_layersOverlay);
		StopRendering();
		SafeRelease(_runtimeName);
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

#if RN_OPENXR_SUPPORTS_PICO_LOADER
		//Pico Neo 3 bindings
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
		xrStringToPath(_internals->instance, "/user/hand/right/input/menu/click", &handRightButtonSystemPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/b/click", &handRightButtonUpperPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/input/a/click", &handRightButtonLowerPressPath);
		xrStringToPath(_internals->instance, "/user/hand/right/output/haptic", &handRightHapticsPath);

		std::vector<XrActionSuggestedBinding> picoNeoBindings;
		picoNeoBindings.push_back({_internals->handLeftAimPoseAction, handLeftAimPosePath});
		picoNeoBindings.push_back({_internals->handLeftGripPoseAction, handLeftGripPosePath});
		picoNeoBindings.push_back({_internals->handLeftTriggerAction, handLeftTriggerPath});
		picoNeoBindings.push_back({_internals->handLeftGrabAction, handLeftGrabPath});
		picoNeoBindings.push_back({_internals->handLeftThumbstickXAction, handLeftThumbstickXPath});
		picoNeoBindings.push_back({_internals->handLeftThumbstickYAction, handLeftThumbstickYPath});
		picoNeoBindings.push_back({_internals->handLeftThumbstickPressAction, handLeftThumbstickPressPath});
		picoNeoBindings.push_back({_internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath});
		picoNeoBindings.push_back({_internals->handLeftButtonUpperPressAction, handLeftButtonUpperPressPath});
		picoNeoBindings.push_back({_internals->handLeftButtonLowerPressAction, handLeftButtonLowerPressPath});
		picoNeoBindings.push_back({_internals->handLeftHapticsAction, handLeftHapticsPath});

		picoNeoBindings.push_back({_internals->handRightAimPoseAction, handRightAimPosePath});
		picoNeoBindings.push_back({_internals->handRightGripPoseAction, handRightGripPosePath});
		picoNeoBindings.push_back({_internals->handRightTriggerAction, handRightTriggerPath});
		picoNeoBindings.push_back({_internals->handRightGrabAction, handRightGrabPath});
		picoNeoBindings.push_back({_internals->handRightThumbstickXAction, handRightThumbstickXPath});
		picoNeoBindings.push_back({_internals->handRightThumbstickYAction, handRightThumbstickYPath});
		picoNeoBindings.push_back({_internals->handRightThumbstickPressAction, handRightThumbstickPressPath});
		picoNeoBindings.push_back({_internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath});
		picoNeoBindings.push_back({_internals->handRightButtonUpperPressAction, handRightButtonUpperPressPath});
		picoNeoBindings.push_back({_internals->handRightButtonLowerPressAction, handRightButtonLowerPressPath});
		picoNeoBindings.push_back({_internals->handRightHapticsAction, handRightHapticsPath});

		xrStringToPath(_internals->instance, "/interaction_profiles/bytedance/pico_neo3_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedPicoNeoBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedPicoNeoBindings.interactionProfile = interactionProfilePath;
		suggestedPicoNeoBindings.suggestedBindings = picoNeoBindings.data();
		suggestedPicoNeoBindings.countSuggestedBindings = picoNeoBindings.size();
		XrResult neoBindingResult = xrSuggestInteractionProfileBindings(_internals->instance, &suggestedPicoNeoBindings);
		if(!XR_SUCCEEDED(neoBindingResult))
		{
			RNDebug("failed action profile suggested binding");
		}


		//Pico 4 bindings
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

		std::vector<XrActionSuggestedBinding> pico4Bindings;
		pico4Bindings.push_back({_internals->handLeftAimPoseAction, handLeftAimPosePath});
		pico4Bindings.push_back({_internals->handLeftGripPoseAction, handLeftGripPosePath});
		pico4Bindings.push_back({_internals->handLeftTriggerAction, handLeftTriggerPath});
		pico4Bindings.push_back({_internals->handLeftGrabAction, handLeftGrabPath});
		pico4Bindings.push_back({_internals->handLeftThumbstickXAction, handLeftThumbstickXPath});
		pico4Bindings.push_back({_internals->handLeftThumbstickYAction, handLeftThumbstickYPath});
		pico4Bindings.push_back({_internals->handLeftThumbstickPressAction, handLeftThumbstickPressPath});
		pico4Bindings.push_back({_internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath});
		pico4Bindings.push_back({_internals->handLeftButtonUpperPressAction, handLeftButtonUpperPressPath});
		pico4Bindings.push_back({_internals->handLeftButtonLowerPressAction, handLeftButtonLowerPressPath});
		pico4Bindings.push_back({_internals->handLeftHapticsAction, handLeftHapticsPath});

		pico4Bindings.push_back({_internals->handRightAimPoseAction, handRightAimPosePath});
		pico4Bindings.push_back({_internals->handRightGripPoseAction, handRightGripPosePath});
		pico4Bindings.push_back({_internals->handRightTriggerAction, handRightTriggerPath});
		pico4Bindings.push_back({_internals->handRightGrabAction, handRightGrabPath});
		pico4Bindings.push_back({_internals->handRightThumbstickXAction, handRightThumbstickXPath});
		pico4Bindings.push_back({_internals->handRightThumbstickYAction, handRightThumbstickYPath});
		pico4Bindings.push_back({_internals->handRightThumbstickPressAction, handRightThumbstickPressPath});
		pico4Bindings.push_back({_internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath});
		pico4Bindings.push_back({_internals->handRightButtonUpperPressAction, handRightButtonUpperPressPath});
		pico4Bindings.push_back({_internals->handRightButtonLowerPressAction, handRightButtonLowerPressPath});
		pico4Bindings.push_back({_internals->handRightHapticsAction, handRightHapticsPath});

		xrStringToPath(_internals->instance, "/interaction_profiles/bytedance/pico4_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedPico4Bindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedPico4Bindings.interactionProfile = interactionProfilePath;
		suggestedPico4Bindings.suggestedBindings = pico4Bindings.data();
		suggestedPico4Bindings.countSuggestedBindings = pico4Bindings.size();
		XrResult pico4BindingResult = xrSuggestInteractionProfileBindings(_internals->instance, &suggestedPico4Bindings);
		if(!XR_SUCCEEDED(pico4BindingResult))
		{
			RNDebug("failed action profile suggested binding");
		}
#endif


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

		uint32 numberOfConfigurationViews = 0;
		if(!XR_SUCCEEDED(xrEnumerateViewConfigurationViews(_internals->instance, _internals->systemID, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &numberOfConfigurationViews, nullptr)))
		{

		}

		XrViewConfigurationView *configurationViews = new XrViewConfigurationView[numberOfConfigurationViews];
		for(uint32 i = 0; i < numberOfConfigurationViews; i++)
		{
			configurationViews[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
			configurationViews[i].next = nullptr;
		}

		if(!XR_SUCCEEDED(xrEnumerateViewConfigurationViews(_internals->instance, _internals->systemID, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, numberOfConfigurationViews, &numberOfConfigurationViews, configurationViews)))
		{

		}

		for(uint32 i = 0; i < numberOfConfigurationViews; i++)
		{
			RNDebug("View: " << configurationViews[i].recommendedImageRectWidth << " x " << configurationViews[i].recommendedImageRectHeight << " : " << configurationViews[i].recommendedSwapchainSampleCount);
		}

		Vector2 eyeRenderSize(configurationViews[0].recommendedImageRectWidth * eyeResolutionFactor, configurationViews[0].recommendedImageRectHeight * eyeResolutionFactor);
		delete[] configurationViews;

		XrResult result = xrCreateSession(_internals->instance, &sessionCreateInfo, &_internals->session);
		if(!XR_SUCCEEDED(result))
		{
			RNDebug("Failed creating OpenXR Session with return value: " << result);
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

#if RN_BUILD_DEBUG
		uint32 numberOfSupportedSwapChainFormats = 0;
		xrEnumerateSwapchainFormats(_internals->session, 0, &numberOfSupportedSwapChainFormats, nullptr);

		int64_t *supportedSwapChainFormats = new int64_t[numberOfSupportedSwapChainFormats];
		xrEnumerateSwapchainFormats(_internals->session, numberOfSupportedSwapChainFormats, &numberOfSupportedSwapChainFormats, supportedSwapChainFormats);

		for(int i = 0; i < numberOfSupportedSwapChainFormats; i++)
		{
			//TODO: Check if the requested swapchain format is actually supported
			RNDebug("Supported swap chain format: " << supportedSwapChainFormats[i]);
		}

		delete[] supportedSwapChainFormats;
#endif

		_internals->views = new XrView[2];
		_internals->views[0].type = XR_TYPE_VIEW;
		_internals->views[0].next = nullptr;
		_internals->views[1].type = XR_TYPE_VIEW;
		_internals->views[1].next = nullptr;

		_mainLayer = new OpenXRCompositorLayer(VRCompositorLayer::Type::TypeProjectionView, descriptor, eyeRenderSize, true, this);

		_mainLayer->_swapChain->_presentEvent = [this](){
			if(_internals->session != XR_NULL_HANDLE && _isSessionRunning)
			{
				std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
				std::vector<XrCompositionLayerProjection> projectionLayers;
				std::vector<XrCompositionLayerQuad> quadLayers;

				//Reserve big enough for them to not resize dynamically as that messes up the pointers that are then passed on...
				projectionLayerViews.reserve(20);
				projectionLayers.reserve(10);
				quadLayers.reserve(10);

				std::vector<XrCompositionLayerBaseHeader*> layers;

				/*XrCompositionLayerSettingsFB layerSettings;
				layerSettings.type = XR_TYPE_COMPOSITION_LAYER_SETTINGS_FB;
				layerSettings.next = nullptr;
				layerSettings.layerFlags = XR_COMPOSITION_LAYER_SETTINGS_AUTO_LAYER_FILTER_BIT_META;*/

				auto insertLayer = [&](OpenXRCompositorLayer *layer) {
					if(!layer->_isActive) return;
					if(layer->_swapChain && !layer->_swapChain->_hasContent) return;

					if(layer->GetType() == VRCompositorLayer::Type::TypeProjectionView)
					{
						XrCompositionLayerProjectionView layerProjectionView;
						layerProjectionView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
						layerProjectionView.next = nullptr;
						layerProjectionView.pose = _internals->views[0].pose;
						layerProjectionView.fov = _internals->views[0].fov;
						layerProjectionView.subImage.swapchain = layer->_swapChain->_internals->swapchain;
						layerProjectionView.subImage.imageRect.offset.x = 0;
						layerProjectionView.subImage.imageRect.offset.y = 0;
						layerProjectionView.subImage.imageRect.extent.width = layer->_swapChain->GetSwapChainSize().x;
						layerProjectionView.subImage.imageRect.extent.height = layer->_swapChain->GetSwapChainSize().y;
						layerProjectionView.subImage.imageArrayIndex = 0;
						projectionLayerViews.push_back(layerProjectionView);

						layerProjectionView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
						layerProjectionView.next = nullptr;
						layerProjectionView.pose = _internals->views[1].pose;
						layerProjectionView.fov = _internals->views[1].fov;
						layerProjectionView.subImage.swapchain = layer->_swapChain->_internals->swapchain;
						layerProjectionView.subImage.imageRect.offset.x = 0;
						layerProjectionView.subImage.imageRect.offset.y = 0;
						layerProjectionView.subImage.imageRect.extent.width = layer->_swapChain->GetSwapChainSize().x;
						layerProjectionView.subImage.imageRect.extent.height = layer->_swapChain->GetSwapChainSize().y;
						layerProjectionView.subImage.imageArrayIndex = 1;
						projectionLayerViews.push_back(layerProjectionView);

						XrCompositionLayerProjection layerProjection;
						layerProjection.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
						layerProjection.next = nullptr;
						layerProjection.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT | XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
						layerProjection.space = _internals->trackingSpace;
						layerProjection.viewCount = 2;
						layerProjection.views = &projectionLayerViews[projectionLayerViews.size() - 2];
						projectionLayers.push_back(layerProjection);

						layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projectionLayers.back()));
					}
					else if(layer->GetType() == VRCompositorLayer::Type::TypeQuad)
					{
						XrCompositionLayerQuad layerQuad;
						layerQuad.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
						layerQuad.next = nullptr;//_supportsCompositionLayerSettings? &layerSettings : nullptr;
						layerQuad.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
						layerQuad.space = _internals->trackingSpace;
						layerQuad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
						layerQuad.subImage.swapchain = layer->_swapChain->_internals->swapchain;
						layerQuad.subImage.imageRect.offset.x = 0;
						layerQuad.subImage.imageRect.offset.y = 0;
						layerQuad.subImage.imageRect.extent.width = layer->_swapChain->GetSwapChainSize().x;
						layerQuad.subImage.imageRect.extent.height = layer->_swapChain->GetSwapChainSize().y;
						layerQuad.subImage.imageArrayIndex = 0;
						layerQuad.pose.position.x = layer->GetPosition().x;
						layerQuad.pose.position.y = layer->GetPosition().y;
						layerQuad.pose.position.z = layer->GetPosition().z;
						layerQuad.pose.orientation.x = layer->GetRotation().x;
						layerQuad.pose.orientation.y = layer->GetRotation().y;
						layerQuad.pose.orientation.z = layer->GetRotation().z;
						layerQuad.pose.orientation.w = layer->GetRotation().w;
						layerQuad.size.width = layer->GetScale().x;
						layerQuad.size.height = layer->GetScale().y;
						quadLayers.push_back(layerQuad);

						layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&quadLayers.back()));
					}
				};

				_layersUnderlay->Enumerate<OpenXRCompositorLayer>([&](OpenXRCompositorLayer *layer, size_t index, bool &stop){
					insertLayer(layer);
				});
				insertLayer(_mainLayer);
				_layersOverlay->Enumerate<OpenXRCompositorLayer>([&](OpenXRCompositorLayer *layer, size_t index, bool &stop){
					insertLayer(layer);
				});

				XrFrameEndInfo frameEndInfo;
				frameEndInfo.type = XR_TYPE_FRAME_END_INFO;
				frameEndInfo.next = nullptr;
				frameEndInfo.displayTime = _internals->predictedDisplayTime;
				frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
				frameEndInfo.layerCount = layers.size();
				frameEndInfo.layers = layers.data();

#if RN_OPENXR_SUPPORTS_METAQUEST_LOADER
				XrLocalDimmingFrameEndInfoMETA xrLocalDimmingFrameEndInfoMETA;
				if(_supportsLocalDimming)
				{
					xrLocalDimmingFrameEndInfoMETA.type = XR_TYPE_LOCAL_DIMMING_FRAME_END_INFO_META;
					xrLocalDimmingFrameEndInfoMETA.localDimmingMode = _isLocalDimmingEnabled? XR_LOCAL_DIMMING_MODE_ON_META : XR_LOCAL_DIMMING_MODE_OFF_META;
					xrLocalDimmingFrameEndInfoMETA.next = nullptr;
					frameEndInfo.next = (void *) &xrLocalDimmingFrameEndInfoMETA;
				}
#endif

				if(XR_FAILED(xrEndFrame(_internals->session, &frameEndInfo)))
				{
					RNDebug("Error in xrEndFrame?");
				}
			}
		};

#if RN_BUILD_DEBUG
		if(_internals->EnumerateDisplayRefreshRatesFB)
		{
			uint32_t numberOfRefreshRates = 0;
			_internals->EnumerateDisplayRefreshRatesFB(_internals->session, 0, &numberOfRefreshRates, nullptr);

			float *refreshRates = new float[numberOfRefreshRates];
			_internals->EnumerateDisplayRefreshRatesFB(_internals->session, numberOfRefreshRates, &numberOfRefreshRates, refreshRates);
			for(int i = 0; i < numberOfRefreshRates; i++)
			{
				RNDebug("Supported Refresh Rate: " << refreshRates[i]);
			}
			delete[] refreshRates;
		}
#endif

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

		_mainLayer->SetFixedFoveatedRenderingLevel(_fixedFoveatedRenderingLevel, _fixedFoveatedRenderingDynamic);

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
		SafeRelease(_mainLayer);
	}

	bool OpenXRWindow::IsRendering() const
	{
		return true;
	}

	void OpenXRWindow::SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic)
	{
		_fixedFoveatedRenderingLevel = level;
		_fixedFoveatedRenderingDynamic = dynamic;

		if(_mainLayer)
		{
			_mainLayer->SetFixedFoveatedRenderingLevel(level, dynamic);
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

	void OpenXRWindow::SetLocalDimming(bool enabled)
	{
		_isLocalDimmingEnabled = (_supportsLocalDimming && enabled);
	}

	Vector2 OpenXRWindow::GetSize() const
	{
		return _mainLayer->GetSize();
	}

	Framebuffer *OpenXRWindow::GetFramebuffer() const
	{
		return _mainLayer->GetFramebuffer();
	}

	Framebuffer *OpenXRWindow::GetFramebuffer(uint8 eye) const
	{
		return _mainLayer->GetFramebuffer();
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
							RNInfo("Session State: Ready");
							XrSessionBeginInfo beginInfo;
							beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
							beginInfo.next = nullptr;
							beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
							xrBeginSession(_internals->session, &beginInfo);

							_isSessionRunning = true;
							_layersUnderlay->Enumerate<OpenXRCompositorLayer>([](OpenXRCompositorLayer *layer, size_t index, bool &stop){
								layer->SetSessionActive(true);
							});
							_mainLayer->SetSessionActive(true);
							_layersOverlay->Enumerate<OpenXRCompositorLayer>([](OpenXRCompositorLayer *layer, size_t index, bool &stop){
								layer->SetSessionActive(true);
							});
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_STOPPING)
						{
							RNInfo("Session State: Stopping");
							_hasSynchronization = false;
							_isSessionRunning = false;
							_layersUnderlay->Enumerate<OpenXRCompositorLayer>([](OpenXRCompositorLayer *layer, size_t index, bool &stop){
								layer->SetSessionActive(false);
							});
							_mainLayer->SetSessionActive(false);
							_layersOverlay->Enumerate<OpenXRCompositorLayer>([](OpenXRCompositorLayer *layer, size_t index, bool &stop){
								layer->SetSessionActive(false);
							});
							xrEndSession(_internals->session);
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_EXITING)
						{
							RNInfo("Session State: Exiting");
							xrDestroySession(_internals->session);
							_internals->session = XR_NULL_HANDLE;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_SYNCHRONIZED)
						{
							RNInfo("Session State: Synchronized");
							_hasSynchronization = true;
							_hasVisibility = false;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_VISIBLE)
						{
							RNInfo("Session State: Visible");
							_hasVisibility = true;
							_hasInputFocus = false;
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_FOCUSED)
						{
							RNInfo("Session State: Focused");
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

						//if(referenceSpaceChangePendingEvent.poseValid)
						{
							//RNDebug("Changed pose: (" << referenceSpaceChangePendingEvent.poseInPreviousSpace.position.x << ", " << referenceSpaceChangePendingEvent.poseInPreviousSpace.position.y << ", " << referenceSpaceChangePendingEvent.poseInPreviousSpace.position.z << ")");
						}

#if RN_OPENXR_SUPPORTS_PICO_LOADER
						_internals->_trackingSpaceCounterRotation = RN::Vector3(_hmdTrackingState.rotation.GetEulerAngle().x, 0.0f, 0.0f);
                        RNInfo("Recenter: " << _internals->_trackingSpaceCounterRotation.GetEulerAngle().x);
#endif

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
#if RN_OPENXR_SUPPORTS_PICO_LOADER
		if(_internals->_supportsControllerInteractionPICO) //This is only gonna be true on a PICO device
		{
			_controllerTrackingState[0].type = VRControllerTrackingState::Type::PicoNeo3Controller;
			_controllerTrackingState[1].type = VRControllerTrackingState::Type::PicoNeo3Controller;
		}
		else
#endif
		{
			_controllerTrackingState[0].type = VRControllerTrackingState::Type::OculusTouchController;
			_controllerTrackingState[1].type = VRControllerTrackingState::Type::OculusTouchController;
		}
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

			if(velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
			{
#if RN_OPENXR_SUPPORTS_PICO_LOADER
				if(_internals->_supportsControllerInteractionPICO)
				{
					//On pico the velocity is somehow wrong after recentering the view, this rotation corrects for that
					//TODO: This will break if they ever fix it...
					_controllerTrackingState[0].velocityLinear = _internals->_trackingSpaceCounterRotation.GetRotatedVector(Vector3(velocity.linearVelocity.x, velocity.linearVelocity.y, velocity.linearVelocity.z));
				}
				else
#endif
				{
					_controllerTrackingState[0].velocityLinear = Vector3(velocity.linearVelocity.x, velocity.linearVelocity.y, velocity.linearVelocity.z);
				}
			}
			else
			{
				//Set velocity to 0, if not valid
				_controllerTrackingState[0].velocityLinear = RN::Vector3();
			}
			if(velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
			{
				_controllerTrackingState[0].velocityAngular = Vector3(velocity.angularVelocity.x, velocity.angularVelocity.y, velocity.angularVelocity.z);
			}
			else
			{
				//Set velocity to 0, if not valid
				_controllerTrackingState[0].velocityAngular = RN::Vector3();
			}

			if(gripLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[0].positionGrip = Vector3(gripLocation.pose.position.x, gripLocation.pose.position.y, gripLocation.pose.position.z);
			}
			if(gripLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[0].rotationGrip = Quaternion(gripLocation.pose.orientation.x, gripLocation.pose.orientation.y, gripLocation.pose.orientation.z, gripLocation.pose.orientation.w);
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
				xrApplyHapticFeedback(_internals->session, &hapticActionInfo, (XrHapticBaseHeader *) &hapticVibration);
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

			if(velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT)
			{
#if RN_OPENXR_SUPPORTS_PICO_LOADER
				if(_internals->_supportsControllerInteractionPICO)
				{
					//On pico the velocity is somehow wrong after recentering the view, this rotation corrects for that
					//TODO: This will break if they ever fix it...
					_controllerTrackingState[1].velocityLinear = _internals->_trackingSpaceCounterRotation.GetRotatedVector(Vector3(velocity.linearVelocity.x, velocity.linearVelocity.y, velocity.linearVelocity.z));
				}
				else
#endif
				{
					_controllerTrackingState[1].velocityLinear = Vector3(velocity.linearVelocity.x, velocity.linearVelocity.y, velocity.linearVelocity.z);
				}
			}
			else
			{
				//Set velocity to 0, if not valid
				_controllerTrackingState[1].velocityLinear = RN::Vector3();
			}

			if(velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)
			{
				_controllerTrackingState[1].velocityAngular = Vector3(velocity.angularVelocity.x, velocity.angularVelocity.y, velocity.angularVelocity.z);
			}
			else
			{
				//Set velocity to 0, if not valid
				_controllerTrackingState[1].velocityAngular = RN::Vector3();
			}

			if(gripLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[1].positionGrip = Vector3(gripLocation.pose.position.x, gripLocation.pose.position.y, gripLocation.pose.position.z);
			}
			if(gripLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[1].rotationGrip = Quaternion(gripLocation.pose.orientation.x, gripLocation.pose.orientation.y, gripLocation.pose.orientation.z, gripLocation.pose.orientation.w);
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
				xrApplyHapticFeedback(_internals->session, &hapticActionInfo,  (XrHapticBaseHeader *) &hapticVibration);
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

	Mesh *OpenXRWindow::GetHiddenAreaMesh(uint8 eye) const
	{
		if(!_supportsVisibilityMask || !_internals->GetVisibilityMaskKHR) return nullptr;
		if(!_internals->session) return nullptr;

		XrVisibilityMaskKHR visibilityMask = {};
		visibilityMask.type = XR_TYPE_VISIBILITY_MASK_KHR;
		visibilityMask.next = nullptr;
		visibilityMask.vertexCapacityInput = 0;
		visibilityMask.indexCapacityInput = 0;
		visibilityMask.vertices = nullptr;
		visibilityMask.indices = nullptr;

		_internals->GetVisibilityMaskKHR(_internals->session, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibilityMask);
		if(visibilityMask.vertexCountOutput == 0) return nullptr;

		visibilityMask.vertexCapacityInput = visibilityMask.vertexCountOutput;
		visibilityMask.indexCapacityInput = visibilityMask.indexCountOutput;
		visibilityMask.vertices = new XrVector2f[visibilityMask.vertexCapacityInput];
		visibilityMask.indices = new uint32_t[visibilityMask.indexCapacityInput];

		_internals->GetVisibilityMaskKHR(_internals->session, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, eye, XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibilityMask);

		Mesh *mesh = new Mesh({Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32), Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2)}, visibilityMask.vertexCountOutput, visibilityMask.indexCountOutput);

		mesh->BeginChanges();
		mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, visibilityMask.vertices);
		mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, visibilityMask.indices);
		mesh->EndChanges();

		delete[] visibilityMask.vertices;
		delete[] visibilityMask.indices;

		return mesh->Autorelease();
	}

	RenderingDevice *OpenXRWindow::GetOutputDevice(RendererDescriptor *descriptor) const
	{
		return nullptr;
	}

	const Window::SwapChainDescriptor &OpenXRWindow::GetSwapChainDescriptor() const
	{
		return _mainLayer->_swapChain->GetSwapChainDescriptor();
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

		RN::Array *result = extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
		return result;
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
		RN::Array *result = extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
		int removeIndex = -1;
		result->Enumerate<String>([&](String *extension, size_t index, bool &stop) {
			if(extension->IsEqual(RNCSTR(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)))
			{
				removeIndex = index;
				stop = true;
			}
		});
		if (removeIndex != -1) result->RemoveObjectAtIndex(removeIndex);
		return result;
#else
		return nullptr;
#endif
	}

	VRWindow::DeviceType OpenXRWindow::GetDeviceType() const
	{
		return _deviceType;
	}

	VRCompositorLayer *OpenXRWindow::CreateCompositorLayer(VRCompositorLayer::Type type, const SwapChainDescriptor &descriptor, RN::Vector2 resolution, bool supportsFoveation)
	{
		OpenXRCompositorLayer *layer = new OpenXRCompositorLayer(type, descriptor, resolution, supportsFoveation, this);
		layer->SetSessionActive(_isSessionRunning);
		return layer->Autorelease();
	}

	void OpenXRWindow::AddCompositorLayer(VRCompositorLayer *layer, bool isUnderlay, bool lowest)
	{
		RN_ASSERT(!_layersUnderlay->ContainsObject(layer) && !_layersOverlay->ContainsObject(layer), "VRCompositorLayer can only be added once!");
		if(isUnderlay)
		{
			if(lowest) _layersUnderlay->InsertObjectAtIndex(layer, 0);
			else _layersUnderlay->AddObject(layer);
		}
		else
		{
			if(lowest) _layersOverlay->InsertObjectAtIndex(layer, 0);
			else _layersOverlay->AddObject(layer);
		}
	}

	void OpenXRWindow::RemoveCompositorLayer(VRCompositorLayer *layer)
	{
		_layersUnderlay->RemoveObject(layer);
		_layersOverlay->RemoveObject(layer);
	}
}

