//
//  RNOpenXRInternals.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_OpenXRINTERNALS_H_
#define __RAYNE_OpenXRINTERNALS_H_

#include "RNOpenXR.h"

#if XR_USE_GRAPHICS_API_VULKAN
#include "RNOpenXRVulkanSwapChain.h"
#endif

#if XR_USE_GRAPHICS_API_D3D12
#include <initguid.h>
#include "RNOpenXRD3D12SwapChain.h"
#include <RND3D12Renderer.h>
#endif

#if XR_USE_GRAPHICS_API_METAL
#include "RNOpenXRMetalSwapChain.h"
#endif

#include "RNOpenXRCompositorLayer.h"

#include "openxr/openxr_platform_defines.h"
#include "openxr/openxr_platform.h"
#include "openxr/openxr.h"

#if XR_USE_PLATFORM_ANDROID
    #if RN_OPENXR_SUPPORTS_PICO_LOADER
		//Include any pico specific extension headers here if they aren't part of the official headers yet
    #elif RN_OPENXR_SUPPORTS_METAQUEST_LOADER
        //Include any meta specific extension headers here if they aren't part of the official headers yet
    #endif
#include "RNOpenXRDispatchTable.h"
#endif

namespace RN
{
	struct OpenXRWindowInternals
	{
		XrInstance instance;
		XrSystemId systemID;
		XrSystemProperties systemProperties;
		XrSession session;

		XrSpace trackingSpace;
		XrTime predictedDisplayTime;

		XrView *views;

		XrActionSet gameActionSet;

		XrAction handLeftAimPoseAction;
		XrAction handLeftGripPoseAction;
		XrAction handLeftTriggerAction;
		XrAction handLeftGrabAction;
		XrAction handLeftThumbstickXAction;
		XrAction handLeftThumbstickYAction;
		XrAction handLeftThumbstickPressAction;
		XrAction handLeftTrackpadXAction;
		XrAction handLeftTrackpadYAction;
		XrAction handLeftTrackpadTouchAction;
		XrAction handLeftTrackpadPressAction;
		XrAction handLeftButtonSystemPressAction;
		XrAction handLeftButtonUpperPressAction;
		XrAction handLeftButtonLowerPressAction;
		XrAction handLeftHapticsAction;
		XrSpace handLeftAimPoseSpace;
		XrSpace handLeftGripPoseSpace;

		XrAction handRightAimPoseAction;
		XrAction handRightGripPoseAction;
		XrAction handRightTriggerAction;
		XrAction handRightGrabAction;
		XrAction handRightThumbstickXAction;
		XrAction handRightThumbstickYAction;
		XrAction handRightThumbstickPressAction;
		XrAction handRightTrackpadXAction;
		XrAction handRightTrackpadYAction;
		XrAction handRightTrackpadTouchAction;
		XrAction handRightTrackpadPressAction;
		XrAction handRightButtonSystemPressAction;
		XrAction handRightButtonUpperPressAction;
		XrAction handRightButtonLowerPressAction;
		XrAction handRightHapticsAction;
		XrSpace handRightAimPoseSpace;
		XrSpace handRightGripPoseSpace;

#if XR_USE_GRAPHICS_API_VULKAN
		PFN_xrGetVulkanInstanceExtensionsKHR GetVulkanInstanceExtensionsKHR;
		PFN_xrGetVulkanDeviceExtensionsKHR GetVulkanDeviceExtensionsKHR;
		PFN_xrGetVulkanGraphicsDeviceKHR GetVulkanGraphicsDeviceKHR;
		PFN_xrGetVulkanGraphicsRequirementsKHR GetVulkanGraphicsRequirementsKHR;
#endif

#if XR_USE_GRAPHICS_API_D3D12
		PFN_xrGetD3D12GraphicsRequirementsKHR GetD3D12GraphicsRequirementsKHR;
#endif
		
#if XR_USE_GRAPHICS_API_METAL
		PFN_xrGetMetalGraphicsRequirementsKHR GetMetalGraphicsRequirementsKHR;
#endif

		PFN_xrEnumerateDisplayRefreshRatesFB EnumerateDisplayRefreshRatesFB;
		PFN_xrGetDisplayRefreshRateFB GetDisplayRefreshRateFB;
		PFN_xrRequestDisplayRefreshRateFB RequestDisplayRefreshRateFB;

		PFN_xrPerfSettingsSetPerformanceLevelEXT PerfSettingsSetPerformanceLevelEXT;

		PFN_xrCreateFoveationProfileFB CreateFoveationProfileFB;
		PFN_xrDestroyFoveationProfileFB DestroyFoveationProfileFB;

		PFN_xrUpdateSwapchainFB UpdateSwapchainFB;
		PFN_xrGetSwapchainStateFB GetSwapchainStateFB;

		PFN_xrGetVisibilityMaskKHR GetVisibilityMaskKHR;
		PFN_xrGetRecommendedLayerResolutionMETA GetRecommendedLayerResolutionMETA;

#if XR_USE_PLATFORM_ANDROID
		PFN_xrSetAndroidApplicationThreadKHR SetAndroidApplicationThreadKHR;
#endif

#if RN_OPENXR_SUPPORTS_PICO_LOADER
		RN::Quaternion _trackingSpaceCounterRotation;
#endif
	};

    struct OpenXRSwapchainInternals
    {
        XrSwapchain swapchain;
        XrFoveationProfileFB currentFoveationProfile;
    };

	struct OpenXRCompositorLayerInternals
	{
		XrCompositionLayerSettingsFB layerSettings;

		//Just keep one of each supported layer around for simplicity.
		XrCompositionLayerProjectionView layerProjectionViews[2];
		XrCompositionLayerProjection layerProjection;

		XrCompositionLayerQuad layerQuad;

		//Will point at the layer above that is in use by this class
		XrCompositionLayerBaseHeader *layerBaseHeader;
	};
}


#endif /* __RAYNE_OpenXRINTERNALS_H_ */
