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

#if XR_USE_GRAPHICS_API_METAL
	#include "RNOpenXRMetalSwapChain.h"
#endif

#include "RNOpenXRCompositorLayer.h"

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"
#include "openxr/openxr_platform_defines.h"

namespace RN
{
	struct OpenXRWindowInternals
	{
		XrInstance instance;
		XrSystemId systemID;
		XrSystemProperties systemProperties;
		XrSession session;

		XrSpace trackingSpace;
		XrTime currentFramePredictedDisplayTime;
		bool currentFrameShouldRender;
		bool currentFrameIsValid;
		bool headTrackingValid;

		XrView *views;

		XrPassthroughFB passthroughSessionFB;

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
		PFN_xrGetVulkanGraphicsDevice2KHR GetVulkanGraphicsDevice2KHR;
		PFN_xrGetVulkanGraphicsRequirements2KHR GetVulkanGraphicsRequirements2KHR;
		PFN_xrCreateVulkanInstanceKHR CreateVulkanInstanceKHR;
		PFN_xrCreateVulkanDeviceKHR CreateVulkanDeviceKHR;
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
		PFN_xrSetTilePropertiesHintMETA SetTilePropertiesHintMETA;

		PFN_xrCreatePassthroughFB CreatePassthroughFB;
		PFN_xrDestroyPassthroughFB DestroyPassthroughFB;
		PFN_xrPassthroughStartFB PassthroughStartFB;
		PFN_xrPassthroughPauseFB PassthroughPauseFB;
		PFN_xrCreatePassthroughLayerFB CreatePassthroughLayerFB;
		PFN_xrDestroyPassthroughLayerFB DestroyPassthroughLayerFB;
		PFN_xrPassthroughLayerPauseFB PassthroughLayerPauseFB;
		PFN_xrPassthroughLayerResumeFB PassthroughLayerResumeFB;
		PFN_xrPassthroughLayerSetStyleFB PassthroughLayerSetStyleFB;

#if XR_USE_PLATFORM_ANDROID
		PFN_xrSetAndroidApplicationThreadKHR SetAndroidApplicationThreadKHR;
#endif

		PFN_xrCreateHandTrackerEXT CreateHandTrackerEXT;
		PFN_xrDestroyHandTrackerEXT DestroyHandTrackerEXT;
		PFN_xrLocateHandJointsEXT LocateHandJointsEXT;
		XrHandTrackerEXT handTracker[2];
		std::vector<XrTilePropertiesMETA> currentTilePropertiesHint;
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

		XrPassthroughLayerFB layerPassthroughFb;
		XrCompositionLayerPassthroughFB layerPassthroughCompFb;

		//Will point at the layer above that is in use by this class
		XrCompositionLayerBaseHeader *layerBaseHeader;
	};
} // namespace RN


#endif /* __RAYNE_OpenXRINTERNALS_H_ */
