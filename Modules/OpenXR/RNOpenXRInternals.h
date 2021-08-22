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

#include "RNOpenXRVulkanSwapChain.h"

#include "openxr/openxr_platform_defines.h"
#include "openxr/openxr_platform.h"
#include "openxr/openxr.h"

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

		XrAction handLeftAction;
		XrAction handLeftTriggerAction;
		XrAction handLeftGrabAction;
		XrAction handLeftThumbstickXAction;
		XrAction handLeftThumbstickYAction;
		XrAction handLeftThumbstickPressAction;
		XrAction handLeftButtonSystemPressAction;
		XrAction handLeftButtonUpperPressAction;
		XrAction handLeftButtonLowerPressAction;
		XrAction handLeftHapticsAction;
		XrSpace handLeftSpace;

		XrAction handRightAction;
		XrAction handRightTriggerAction;
		XrAction handRightGrabAction;
		XrAction handRightThumbstickXAction;
		XrAction handRightThumbstickYAction;
		XrAction handRightThumbstickPressAction;
		XrAction handRightButtonSystemPressAction;
		XrAction handRightButtonUpperPressAction;
		XrAction handRightButtonLowerPressAction;
		XrAction handRightHapticsAction;
		XrSpace handRightSpace;

		PFN_xrGetVulkanInstanceExtensionsKHR GetVulkanInstanceExtensionsKHR;
		PFN_xrGetVulkanDeviceExtensionsKHR GetVulkanDeviceExtensionsKHR;
		PFN_xrGetVulkanGraphicsDeviceKHR GetVulkanGraphicsDeviceKHR;
		PFN_xrGetVulkanGraphicsRequirementsKHR GetVulkanGraphicsRequirementsKHR;

		PFN_xrEnumerateDisplayRefreshRatesFB EnumerateDisplayRefreshRatesFB;
		PFN_xrGetDisplayRefreshRateFB GetDisplayRefreshRateFB;
		PFN_xrRequestDisplayRefreshRateFB RequestDisplayRefreshRateFB;

		PFN_xrPerfSettingsSetPerformanceLevelEXT PerfSettingsSetPerformanceLevelEXT;

		PFN_xrCreateFoveationProfileFB CreateFoveationProfileFB;
		PFN_xrDestroyFoveationProfileFB DestroyFoveationProfileFB;

		PFN_xrUpdateSwapchainFB UpdateSwapchainFB;
		PFN_xrGetSwapchainStateFB GetSwapchainStateFB;

		PFN_xrSetAndroidApplicationThreadKHR SetAndroidApplicationThreadKHR;
	};

    struct OpenXRSwapchainInternals
    {
        XrSwapchain swapchain;
        XrFoveationProfileFB currentFoveationProfile;
    };
}


#endif /* __RAYNE_OpenXRINTERNALS_H_ */
