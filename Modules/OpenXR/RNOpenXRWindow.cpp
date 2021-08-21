//
//  RNOpenXRWindow.cpp
//  Rayne-OpenXR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenXRWindow.h"
#include "RNOpenXRInternals.h"

#include <unistd.h>
#include <android/log.h>

#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <android/native_window_jni.h> // for native window JNI
#include <android_native_app_glue.h>

namespace RN
{
	RNDefineMeta(OpenXRWindow, VRWindow)

	OpenXRWindow::OpenXRWindow() : _nativeWindow(nullptr), _internals(new OpenXRWindowInternals()), _session(nullptr), _swapChain(nullptr), _actualFrameIndex(0), _predictedDisplayTime(0.0), _currentHapticsIndex{0, 0}, _preferredFrameRate(0), _minCPULevel(0), _minGPULevel(0), _fixedFoveatedRenderingLevel(2), _fixedFoveatedRenderingDynamic(false), _hasInputFocus(true), _hasVisibility(true)
	{
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

		XrBaseInStructure *platformSpecificInstanceCreateInfo = nullptr;

        // Create required extensions array
        std::vector<const char*> extensions;
#if RN_PLATFORM_ANDROID
		extensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
		extensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);

		XrInstanceCreateInfoAndroidKHR instanceCreateInfo = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
		instanceCreateInfo.applicationVM = app->activity->vm;
		instanceCreateInfo.applicationActivity = app->activity->clazz;

		platformSpecificInstanceCreateInfo = reinterpret_cast<XrBaseInStructure*>(&instanceCreateInfo);
#endif

        uint32_t instanceExtensionCount;
        xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr);
        std::vector<XrExtensionProperties> allExtensions(instanceExtensionCount);
        for(XrExtensionProperties& extension : allExtensions)
        {
            extension.type = XR_TYPE_EXTENSION_PROPERTIES;
        }
        xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)allExtensions.size(), &instanceExtensionCount, allExtensions.data());

        RNDebug("Available Extensions (" << instanceExtensionCount << "):");
        for(const XrExtensionProperties& extension : allExtensions)
        {
            RNDebug("  Name: " << extension.extensionName << ", Spec Version: " << extension.extensionVersion);
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

        _mainThreadID = gettid();
		_hmdTrackingState.type = (_internals->systemProperties.trackingProperties.orientationTracking && _internals->systemProperties.trackingProperties.positionTracking)? VRHMDTrackingState::Type::SixDegreesOfFreedom : VRHMDTrackingState::Type::ThreeDegreesOfFreedom;
		RNInfo(GetHMDInfoDescription());


        //TODO: Only load these if vulkan_enable extension is supported (there is also a vulkan_enable2 extension, not supported by quest)
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

	OpenXRWindow::~OpenXRWindow()
	{
		StopRendering();
		xrDestroyInstance(_internals->instance);
		delete _internals;
	}

	void OpenXRWindow::StartRendering(const SwapChainDescriptor &descriptor)
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

		uint32_t numberOfConfigurationViews = 0;
		if(!XR_SUCCEEDED(xrEnumerateViewConfigurationViews(_internals->instance, _internals->systemID, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &numberOfConfigurationViews, nullptr)))
		{

		}

		XrViewConfigurationView *configurationViews = new XrViewConfigurationView[numberOfConfigurationViews];
		if(!XR_SUCCEEDED(xrEnumerateViewConfigurationViews(_internals->instance, _internals->systemID, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, numberOfConfigurationViews, &numberOfConfigurationViews, configurationViews)))
		{

		}

		for(int i = 0; i < numberOfConfigurationViews; i++)
		{
			RNDebug("View: " << configurationViews[i].recommendedImageRectWidth << " x " << configurationViews[i].recommendedImageRectHeight << " : " << configurationViews[i].recommendedSwapchainSampleCount);
		}

		//1:1 mapping for center according to docs would be 1536x1536, returned is 1024*1024 for GO, higher on quest
		Vector2 eyeRenderSize(configurationViews[0].recommendedImageRectWidth, configurationViews[0].recommendedImageRectHeight);
		delete[] configurationViews;

		XrGraphicsBindingVulkanKHR vulkanGraphicsBinding;
		vulkanGraphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
		vulkanGraphicsBinding.instance = renderer->GetVulkanInstance()->GetInstance();
		vulkanGraphicsBinding.physicalDevice = renderer->GetVulkanDevice()->GetPhysicalDevice();
		vulkanGraphicsBinding.device = renderer->GetVulkanDevice()->GetDevice();
		vulkanGraphicsBinding.queueFamilyIndex = renderer->GetVulkanDevice()->GetWorkQueue();
		vulkanGraphicsBinding.queueIndex = 0; //There should be only one queue at the moment, so it's index should be 0...
		vulkanGraphicsBinding.next = nullptr;

		XrSessionCreateInfo sessionCreateInfo;
		sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
		sessionCreateInfo.createFlags = 0;
		sessionCreateInfo.systemId = _internals->systemID;
		sessionCreateInfo.next = &vulkanGraphicsBinding;
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

		_swapChain = new OpenXRVulkanSwapChain(this, descriptor, eyeRenderSize);

		_swapChain->_presentEvent = [this](){
			if(_internals->session != XR_NULL_HANDLE)
			{
                XrCompositionLayerProjectionView layerProjectionView[2];
                layerProjectionView[0].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                layerProjectionView[0].next = nullptr;
                layerProjectionView[0].pose = _internals->views[0].pose;
                layerProjectionView[0].fov = _internals->views[0].fov;
                layerProjectionView[0].subImage.swapchain = _swapChain->_internals->swapchain;
                layerProjectionView[0].subImage.imageRect.offset.x = 0;
                layerProjectionView[0].subImage.imageRect.offset.y = 0;
                layerProjectionView[0].subImage.imageRect.extent.width = _swapChain->_size.x;
                layerProjectionView[0].subImage.imageRect.extent.height = _swapChain->_size.y;
                layerProjectionView[0].subImage.imageArrayIndex = 0;

                layerProjectionView[1].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
                layerProjectionView[1].next = nullptr;
                layerProjectionView[1].pose = _internals->views[1].pose;
                layerProjectionView[1].fov = _internals->views[1].fov;
                layerProjectionView[1].subImage.swapchain = _swapChain->_internals->swapchain;
                layerProjectionView[1].subImage.imageRect.offset.x = 0;
                layerProjectionView[1].subImage.imageRect.offset.y = 0;
                layerProjectionView[1].subImage.imageRect.extent.width = _swapChain->_size.x;
                layerProjectionView[1].subImage.imageRect.extent.height = _swapChain->_size.y;
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

		NotificationManager::GetSharedInstance()->AddSubscriber(kRNAndroidWindowDidChange, [this](Notification *notification) {
				if(notification->GetName()->IsEqual(kRNAndroidWindowDidChange))
				{
					UpdateVRMode();
				}
			}, this);

		UpdateVRMode();


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
		XrActionCreateInfo handLeftActionInfo;
		handLeftActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
		handLeftActionInfo.next = nullptr;
		strcpy(handLeftActionInfo.actionName, "hand_left");
		handLeftActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(handLeftActionInfo.localizedActionName, "Hand Left");
		handLeftActionInfo.countSubactionPaths = 0;
		handLeftActionInfo.subactionPaths = nullptr;
		if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handLeftActionInfo, &_internals->handLeftAction)))
		{
			RN_ASSERT(false, "failed creating left hand action");
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

		XrPath handLeftPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/aim/pose", &handLeftPath);

		XrPath handLeftTriggerPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/trigger/value", &handLeftTriggerPath);

		XrPath handLeftGrabPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/squeeze/value", &handLeftGrabPath);

		XrPath handLeftThumbstickXPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/x", &handLeftThumbstickXPath);

		XrPath handLeftThumbstickYPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/y", &handLeftThumbstickYPath);

		XrPath handLeftThumbstickPressPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/thumbstick/click", &handLeftThumbstickPressPath);

		XrPath handLeftButtonSystemPressPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/menu/click", &handLeftButtonSystemPressPath);

		XrPath handLeftButtonUpperPressPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/y/click", &handLeftButtonUpperPressPath);

		XrPath handLeftButtonLowerPressPath;
		xrStringToPath(_internals->instance, "/user/hand/left/input/x/click", &handLeftButtonLowerPressPath);

        //Right hand
        XrActionCreateInfo handRightActionInfo;
        handRightActionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
        handRightActionInfo.next = nullptr;
        strcpy(handRightActionInfo.actionName, "hand_right");
        handRightActionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
        strcpy(handRightActionInfo.localizedActionName, "Hand Right");
        handRightActionInfo.countSubactionPaths = 0;
        handRightActionInfo.subactionPaths = nullptr;
        if(!XR_SUCCEEDED(xrCreateAction(_internals->gameActionSet, &handRightActionInfo, &_internals->handRightAction)))
        {
            RN_ASSERT(false, "failed creating right hand action");
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

        XrPath handRightPath;
        xrStringToPath(_internals->instance, "/user/hand/right/input/aim/pose", &handRightPath);

        XrPath handRightTriggerPath;
        xrStringToPath(_internals->instance, "/user/hand/right/input/trigger/value", &handRightTriggerPath);

        XrPath handRightGrabPath;
        xrStringToPath(_internals->instance, "/user/hand/right/input/squeeze/value", &handRightGrabPath);

		XrPath handRightThumbstickXPath;
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/x", &handRightThumbstickXPath);

		XrPath handRightThumbstickYPath;
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/y", &handRightThumbstickYPath);

		XrPath handRightThumbstickPressPath;
		xrStringToPath(_internals->instance, "/user/hand/right/input/thumbstick/click", &handRightThumbstickPressPath);

		XrPath handRightButtonSystemPressPath;
		xrStringToPath(_internals->instance, "/user/hand/right/input/system/click", &handRightButtonSystemPressPath);

		XrPath handRightButtonUpperPressPath;
		xrStringToPath(_internals->instance, "/user/hand/right/input/b/click", &handRightButtonUpperPressPath);

		XrPath handRightButtonLowerPressPath;
		xrStringToPath(_internals->instance, "/user/hand/right/input/a/click", &handRightButtonLowerPressPath);

		std::vector<XrActionSuggestedBinding> bindings;
		bindings.push_back({_internals->handLeftAction, handLeftPath});
		bindings.push_back({_internals->handLeftTriggerAction, handLeftTriggerPath});
		bindings.push_back({_internals->handLeftGrabAction, handLeftGrabPath});
		bindings.push_back({_internals->handLeftThumbstickXAction, handLeftThumbstickXPath});
		bindings.push_back({_internals->handLeftThumbstickYAction, handLeftThumbstickYPath});
		bindings.push_back({_internals->handLeftThumbstickPressAction, handLeftThumbstickPressPath});
		bindings.push_back({_internals->handLeftButtonSystemPressAction, handLeftButtonSystemPressPath});
		bindings.push_back({_internals->handLeftButtonUpperPressAction, handLeftButtonUpperPressPath});
		bindings.push_back({_internals->handLeftButtonLowerPressAction, handLeftButtonLowerPressPath});

		bindings.push_back({_internals->handRightAction, handRightPath});
		bindings.push_back({_internals->handRightTriggerAction, handRightTriggerPath});
		bindings.push_back({_internals->handRightGrabAction, handRightGrabPath});
		bindings.push_back({_internals->handRightThumbstickXAction, handRightThumbstickXPath});
		bindings.push_back({_internals->handRightThumbstickYAction, handRightThumbstickYPath});
		bindings.push_back({_internals->handRightThumbstickPressAction, handRightThumbstickPressPath});
		bindings.push_back({_internals->handRightButtonSystemPressAction, handRightButtonSystemPressPath});
		bindings.push_back({_internals->handRightButtonUpperPressAction, handRightButtonUpperPressPath});
		bindings.push_back({_internals->handRightButtonLowerPressAction, handRightButtonLowerPressPath});

        XrPath interactionProfilePath;
        xrStringToPath(_internals->instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);

		XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
		suggestedBindings.interactionProfile = interactionProfilePath;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = bindings.size();
		if(!XR_SUCCEEDED(xrSuggestInteractionProfileBindings(_internals->instance, &suggestedBindings)))
		{
			RN_ASSERT(false, "failed action profile binding");
		}

		XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
		attachInfo.countActionSets = 1;
		attachInfo.actionSets = &_internals->gameActionSet;
		if(!XR_SUCCEEDED(xrAttachSessionActionSets(_internals->session, &attachInfo)))
		{
			RN_ASSERT(false, "failed attaching action sets");
		}

		XrActionSpaceCreateInfo handLeftSpaceCreateInfo;
		handLeftSpaceCreateInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
		handLeftSpaceCreateInfo.next = nullptr;
		handLeftSpaceCreateInfo.action = _internals->handLeftAction;
		handLeftSpaceCreateInfo.subactionPath = XR_NULL_PATH;
		handLeftSpaceCreateInfo.poseInActionSpace.orientation.x = 0.0f;
		handLeftSpaceCreateInfo.poseInActionSpace.orientation.y = 0.0f;
		handLeftSpaceCreateInfo.poseInActionSpace.orientation.z = 0.0f;
		handLeftSpaceCreateInfo.poseInActionSpace.orientation.w = 1.0f;
		handLeftSpaceCreateInfo.poseInActionSpace.position.x = 0.0f;
		handLeftSpaceCreateInfo.poseInActionSpace.position.y = 0.0f;
		handLeftSpaceCreateInfo.poseInActionSpace.position.z = 0.0f;
		xrCreateActionSpace(_internals->session, &handLeftSpaceCreateInfo, &_internals->handLeftSpace);

        XrActionSpaceCreateInfo handRightSpaceCreateInfo;
        handRightSpaceCreateInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        handRightSpaceCreateInfo.next = nullptr;
        handRightSpaceCreateInfo.action = _internals->handRightAction;
        handRightSpaceCreateInfo.subactionPath = XR_NULL_PATH;
        handRightSpaceCreateInfo.poseInActionSpace.orientation.x = 0.0f;
        handRightSpaceCreateInfo.poseInActionSpace.orientation.y = 0.0f;
        handRightSpaceCreateInfo.poseInActionSpace.orientation.z = 0.0f;
        handRightSpaceCreateInfo.poseInActionSpace.orientation.w = 1.0f;
        handRightSpaceCreateInfo.poseInActionSpace.position.x = 0.0f;
        handRightSpaceCreateInfo.poseInActionSpace.position.y = 0.0f;
        handRightSpaceCreateInfo.poseInActionSpace.position.z = 0.0f;
        xrCreateActionSpace(_internals->session, &handRightSpaceCreateInfo, &_internals->handRightSpace);
	}

	void OpenXRWindow::StopRendering()
	{
		NotificationManager::GetSharedInstance()->RemoveSubscriber(kRNAndroidWindowDidChange, this);

		if(_internals->session != XR_NULL_HANDLE)
		{
		//	vrapi_LeaveVrMode(static_cast<ovrMobile*>(_session));
		}

        delete[] _internals->views;

		SafeRelease(_swapChain);

		//vrapi_DestroySystemVulkan();
	}

	bool OpenXRWindow::IsRendering() const
	{
		return true;
	}

	void OpenXRWindow::SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic)
	{
		_fixedFoveatedRenderingLevel = level;
		_fixedFoveatedRenderingDynamic = dynamic;

//		vrapi_SetPropertyInt(static_cast<ovrJava*>(_java), VRAPI_FOVEATION_LEVEL, level);
//		vrapi_SetPropertyInt(static_cast<ovrJava*>(_java), VRAPI_DYNAMIC_FOVEATION_ENABLED, dynamic);
	}

	void OpenXRWindow::SetPreferredFramerate(uint32 framerate)
	{
		_preferredFrameRate = framerate;

		if(_internals->session != XR_NULL_HANDLE)
		{
//			vrapi_SetDisplayRefreshRate(static_cast<ovrMobile*>(_session), framerate);
		}
	}

	void OpenXRWindow::SetPerformanceLevel(uint8 cpuLevel, uint8 gpuLevel)
	{
		_minCPULevel = cpuLevel;
		_minGPULevel = gpuLevel;

		if(_internals->session != XR_NULL_HANDLE)
		{
//			vrapi_SetClockLevels(static_cast<ovrMobile*>(_session), cpuLevel, gpuLevel);
		}
	}

	Vector2 OpenXRWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *OpenXRWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}

	Framebuffer *OpenXRWindow::GetFramebuffer(uint8 eye) const
	{
		RN_ASSERT(eye < 2, "Eye Index need to be 0 or 1");
		return _swapChain->GetFramebuffer();
	}

	static Matrix GetProjectionMatrixForXRFovf(const XrFovf &fov, float near, float far)
	{
		float tan_left = tanf(fov.angleLeft);
		float tan_right = tanf(fov.angleRight);

		float tan_down = tanf(fov.angleDown);
		float tan_up = tanf(fov.angleUp);

		float tan_width = tan_right - tan_left;
		float tan_height = tan_up - tan_down;

		float a11 = 2 / tan_width;
		float a22 = 2 / tan_height;

		float a31 = (tan_right + tan_left) / tan_width;
		float a32 = (tan_up + tan_down) / tan_height;
		float a33 = -far / (far - near);

		float a43 = -(far * near) / (far - near);

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

/*	static Vector3 GetVectorForOVRVector(const ovrVector3f &ovrVector)
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
	}*/

	void OpenXRWindow::BeginFrame(float delta)
	{
		if(_internals->session == XR_NULL_HANDLE) return;

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
							XrSessionBeginInfo beginInfo;
							beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
							beginInfo.next = nullptr;
							beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
							xrBeginSession(_internals->session, &beginInfo);

							RNDebug("Session State: Ready");
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_STOPPING)
						{
							xrEndSession(_internals->session);
                            RNDebug("Session State: Stopping");
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_EXITING)
						{
							xrDestroySession(_internals->session);
                            RNDebug("Session State: Exiting");
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_SYNCHRONIZED)
						{
							_hasVisibility = false;
                            RNDebug("Session State: Synchronized");
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_VISIBLE)
						{
							_hasVisibility = true;
							_hasInputFocus = false;
                            RNDebug("Session State: Visible");
						}
						else if(sessionStateChangedEvent.state == XR_SESSION_STATE_FOCUSED)
						{
							_hasInputFocus = true;
                            RNDebug("Session State: Focused");
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
				}
			}
			else
			{
				break;
			}
		}

        if(_internals->session == XR_NULL_HANDLE) return;

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
        _hmdTrackingState.eyeProjection[0] = GetProjectionMatrixForXRFovf(_internals->views[0].fov, 0.1f, 1000.0f);
        _hmdTrackingState.eyeProjection[1] = GetProjectionMatrixForXRFovf(_internals->views[1].fov, 0.1f, 1000.0f);

		if(_hasVisibility && _hasInputFocus)
		{
			_hmdTrackingState.mode = VRHMDTrackingState::Mode::Rendering;
		}

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

		XrActiveActionSet activeActionSet{_internals->gameActionSet, XR_NULL_PATH};
		XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
		syncInfo.countActiveActionSets = 1;
		syncInfo.activeActionSets = &activeActionSet;
		xrSyncActions(_internals->session, &syncInfo);

		//Left hand
		XrActionStatePose handLeftState{XR_TYPE_ACTION_STATE_POSE};
		XrActionStateGetInfo getHandLeftInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        getHandLeftInfo.action = _internals->handLeftAction;
		xrGetActionStatePose(_internals->session, &getHandLeftInfo, &handLeftState);

		_controllerTrackingState[0].active = handLeftState.isActive;
		_controllerTrackingState[0].tracking = handLeftState.isActive;
		if(handLeftState.isActive)
		{
			XrSpaceVelocity velocity {XR_TYPE_SPACE_VELOCITY};
			XrSpaceLocation location {XR_TYPE_SPACE_LOCATION, &velocity};
			xrLocateSpace(_internals->handLeftSpace, _internals->trackingSpace, _internals->predictedDisplayTime, &location);

			if(location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
			{
				_controllerTrackingState[0].position = Vector3(location.pose.position.x, location.pose.position.y, location.pose.position.z);
			}
			if(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
			{
				_controllerTrackingState[0].rotation = Quaternion(location.pose.orientation.x, location.pose.orientation.y, location.pose.orientation.z, location.pose.orientation.w);
				_controllerTrackingState[0].rotation *= RN::Vector3(0.0f, 45.0f, 0.0f);
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

			XrActionStateBoolean handButtonSystemPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonSystemPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonSystemPressGetInfo.action = _internals->handLeftButtonSystemPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonSystemPressGetInfo, &handButtonSystemPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::Start] = handButtonSystemPressState.currentState;

			XrActionStateBoolean handButtonUpperPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonUpperPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonUpperPressGetInfo.action = _internals->handLeftButtonUpperPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonUpperPressGetInfo, &handButtonUpperPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::BY] = handButtonUpperPressState.currentState;

			XrActionStateBoolean handButtonLowerPressState{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo handButtonLowerPressGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			handButtonLowerPressGetInfo.action = _internals->handLeftButtonLowerPressAction;
			xrGetActionStateBoolean(_internals->session, &handButtonLowerPressGetInfo, &handButtonLowerPressState);
			_controllerTrackingState[0].button[VRControllerTrackingState::Button::AX] = handButtonLowerPressState.currentState;
		}

		//Right hand
        XrActionStatePose handRightState{XR_TYPE_ACTION_STATE_POSE};
        XrActionStateGetInfo getHandRightInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        getHandRightInfo.action = _internals->handRightAction;
        xrGetActionStatePose(_internals->session, &getHandRightInfo, &handRightState);

        _controllerTrackingState[1].active = handRightState.isActive;
        _controllerTrackingState[1].tracking = handRightState.isActive;
        if(handRightState.isActive)
        {
            XrSpaceVelocity velocity {XR_TYPE_SPACE_VELOCITY};
            XrSpaceLocation location {XR_TYPE_SPACE_LOCATION, &velocity};
            xrLocateSpace(_internals->handRightSpace, _internals->trackingSpace, _internals->predictedDisplayTime, &location);

            if(location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
            {
                _controllerTrackingState[1].position = Vector3(location.pose.position.x, location.pose.position.y, location.pose.position.z);
            }
            if(location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)
            {
                _controllerTrackingState[1].rotation = Quaternion(location.pose.orientation.x, location.pose.orientation.y, location.pose.orientation.z, location.pose.orientation.w);
				_controllerTrackingState[1].rotation *= RN::Vector3(0.0f, 45.0f, 0.0f);
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
        }

/*		ovrInputCapabilityHeader capsHeader;
		int i = 0;
		while(vrapi_EnumerateInputDevices(static_cast<ovrMobile*>(_session), i, &capsHeader) >= 0)
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

					if(_currentHapticsIndex[handIndex] < _haptics[handIndex].sampleCount)
					{
						float strength = _haptics[handIndex].samples[_currentHapticsIndex[handIndex]++];
						vrapi_SetHapticVibrationSimple(static_cast<ovrMobile*>(_session), _controllerTrackingState[handIndex].controllerID, strength);
					}
					else
					{
						vrapi_SetHapticVibrationSimple(static_cast<ovrMobile*>(_session), _controllerTrackingState[handIndex].controllerID, 0.0f);
					}
				}
			}
			else if(capsHeader.Type == ovrControllerType_Hand)
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

	void OpenXRWindow::UpdateVRMode()
	{
		RNDebug(RNCSTR("UpdateVRMode called"));

/*		if(!_nativeWindow)
		{
			if(!_session)
			{
				android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
				_nativeWindow = app->window;

				VulkanRenderer *renderer = Renderer::GetActiveRenderer()->Downcast<VulkanRenderer>();
				ovrModeParmsVulkan params = vrapi_DefaultModeParmsVulkan(static_cast<ovrJava*>(_java), (unsigned long long)renderer->GetWorkQueue());
				params.ModeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW | VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB | VRAPI_MODE_FLAG_PHASE_SYNC;
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

					int refreshRateCount = vrapi_GetSystemPropertyInt(static_cast<ovrJava*>(_java), VRAPI_SYS_PROP_NUM_SUPPORTED_DISPLAY_REFRESH_RATES);
					float *availableRefreshRates = new float[refreshRateCount];
					vrapi_GetSystemPropertyFloatArray(static_cast<ovrJava*>(_java), VRAPI_SYS_PROP_SUPPORTED_DISPLAY_REFRESH_RATES, availableRefreshRates, refreshRateCount);
					float highestRefreshRate = _preferredFrameRate;
					if(_preferredFrameRate == 0)
					{
						highestRefreshRate = availableRefreshRates[0];
						for(int i = 0; i < refreshRateCount; i++)
						{
							RNDebug("Available Refresh Rate: " << availableRefreshRates[i]);
							if(availableRefreshRates[i] > highestRefreshRate)
							{
								highestRefreshRate = availableRefreshRates[i];
							}
						}
					}

					vrapi_SetDisplayRefreshRate(session, highestRefreshRate);
					vrapi_SetClockLevels(session, _minCPULevel, _minGPULevel); //TODO: Set to 0, 0 for automatic clock levels, current setting keeps optimizations more comparable
					vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_MAIN, _mainThreadID);
    				vrapi_SetPerfThread(session, VRAPI_PERF_THREAD_TYPE_RENDERER, _mainThreadID);

					vrapi_SetExtraLatencyMode(session, VRAPI_EXTRA_LATENCY_MODE_ON);

					vrapi_SetPropertyInt(static_cast<ovrJava*>(_java), VRAPI_DYNAMIC_FOVEATION_ENABLED, _fixedFoveatedRenderingDynamic);
					vrapi_SetPropertyInt(static_cast<ovrJava*>(_java), VRAPI_FOVEATION_LEVEL, _fixedFoveatedRenderingLevel);
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
		char names[4096];
		uint32_t size = sizeof(names);
		if(_internals->GetVulkanInstanceExtensionsKHR(_internals->instance, _internals->systemID, size, &size, names) != XR_SUCCESS)
		{
			return nullptr;
		}

        String *extensionString = RNSTR(names);
		RNDebug("Needs vulkan instance extensions: " << extensionString);
        return extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
	}

    Array *OpenXRWindow::GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *device) const
    {
		char names[4096];
		uint32_t size = sizeof(names);
		if(_internals->GetVulkanDeviceExtensionsKHR(_internals->instance, _internals->systemID, size, &size, names) != XR_SUCCESS)
		{
			return nullptr;
		}

		String *extensionString = RNSTR(names);
		RNDebug("Needs vulkan device extensions: " << extensionString);
		return extensionString->GetComponentsSeparatedByString(RNCSTR(" "));
    }

	VRWindow::DeviceType OpenXRWindow::GetDeviceType() const
	{
		/*ovrJava *java = static_cast<ovrJava*>(_java);
		int deviceType = vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_DEVICE_TYPE);
		if(deviceType > VRAPI_DEVICE_TYPE_OCULUSQUEST_END)
		{
			return VRWindow::DeviceType::OculusQuest2;
		}
		else
		{
			return VRWindow::DeviceType::OculusQuest;
		}*/

		return VRWindow::DeviceType::OculusQuest2;
	}
}

