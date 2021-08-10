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

