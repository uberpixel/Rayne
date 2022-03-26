// Copyright (c) 2017-2021, The Khronos Group Inc.
// Copyright (c) 2017-2019 Valve Corporation
// Copyright (c) 2017-2019 LunarG, Inc.
// SPDX-License-Identifier: Apache-2.0 OR MIT
// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********
//     See utility_source_generator.py for modifications
// ************************************************************

// Copyright (c) 2017-2021, The Khronos Group Inc.
// Copyright (c) 2017-2019 Valve Corporation
// Copyright (c) 2017-2019 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Mark Young <marky@lunarg.com>
//

#pragma once
#include "xr_dependencies.h"
#include "openxr/openxr_platform_defines.h"
#include "openxr/openxr_platform.h"
#include "openxr/openxr.h"

#ifdef __cplusplus
extern "C" { 
#endif
// Generated dispatch table
//struct XrGeneratedDispatchTable {

    // ---- Core 1.0 commands
    extern PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr;
    extern PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
    extern PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
    extern PFN_xrCreateInstance xrCreateInstance;
    extern PFN_xrDestroyInstance xrDestroyInstance;
    extern PFN_xrGetInstanceProperties xrGetInstanceProperties;
    extern PFN_xrPollEvent xrPollEvent;
    extern PFN_xrResultToString xrResultToString;
    extern PFN_xrStructureTypeToString xrStructureTypeToString;
    extern PFN_xrGetSystem xrGetSystem;
    extern PFN_xrGetSystemProperties xrGetSystemProperties;
    extern PFN_xrEnumerateEnvironmentBlendModes xrEnumerateEnvironmentBlendModes;
    extern PFN_xrCreateSession xrCreateSession;
    extern PFN_xrDestroySession xrDestroySession;
    extern PFN_xrEnumerateReferenceSpaces xrEnumerateReferenceSpaces;
    extern PFN_xrCreateReferenceSpace xrCreateReferenceSpace;
    extern PFN_xrGetReferenceSpaceBoundsRect xrGetReferenceSpaceBoundsRect;
    extern PFN_xrCreateActionSpace xrCreateActionSpace;
    extern PFN_xrLocateSpace xrLocateSpace;
    extern PFN_xrDestroySpace xrDestroySpace;
    extern PFN_xrEnumerateViewConfigurations xrEnumerateViewConfigurations;
    extern PFN_xrGetViewConfigurationProperties xrGetViewConfigurationProperties;
    extern PFN_xrEnumerateViewConfigurationViews xrEnumerateViewConfigurationViews;
    extern PFN_xrEnumerateSwapchainFormats xrEnumerateSwapchainFormats;
    extern PFN_xrCreateSwapchain xrCreateSwapchain;
    extern PFN_xrDestroySwapchain xrDestroySwapchain;
    extern PFN_xrEnumerateSwapchainImages xrEnumerateSwapchainImages;
    extern PFN_xrAcquireSwapchainImage xrAcquireSwapchainImage;
    extern PFN_xrWaitSwapchainImage xrWaitSwapchainImage;
    extern PFN_xrReleaseSwapchainImage xrReleaseSwapchainImage;
    extern PFN_xrBeginSession xrBeginSession;
    extern PFN_xrEndSession xrEndSession;
    extern PFN_xrRequestExitSession xrRequestExitSession;
    extern PFN_xrWaitFrame xrWaitFrame;
    extern PFN_xrBeginFrame xrBeginFrame;
    extern PFN_xrEndFrame xrEndFrame;
    extern PFN_xrLocateViews xrLocateViews;
    extern PFN_xrStringToPath xrStringToPath;
    extern PFN_xrPathToString xrPathToString;
    extern PFN_xrCreateActionSet xrCreateActionSet;
    extern PFN_xrDestroyActionSet xrDestroyActionSet;
    extern PFN_xrCreateAction xrCreateAction;
    extern PFN_xrDestroyAction xrDestroyAction;
    extern PFN_xrSuggestInteractionProfileBindings xrSuggestInteractionProfileBindings;
    extern PFN_xrAttachSessionActionSets xrAttachSessionActionSets;
    extern PFN_xrGetCurrentInteractionProfile xrGetCurrentInteractionProfile;
    extern PFN_xrGetActionStateBoolean xrGetActionStateBoolean;
    extern PFN_xrGetActionStateFloat xrGetActionStateFloat;
    extern PFN_xrGetActionStateVector2f xrGetActionStateVector2f;
    extern PFN_xrGetActionStatePose xrGetActionStatePose;
    extern PFN_xrSyncActions xrSyncActions;
    extern PFN_xrEnumerateBoundSourcesForAction xrEnumerateBoundSourcesForAction;
    extern PFN_xrGetInputSourceLocalizedName xrGetInputSourceLocalizedName;
    extern PFN_xrApplyHapticFeedback xrApplyHapticFeedback;
    extern PFN_xrStopHapticFeedback xrStopHapticFeedback;

    // ---- XR_KHR_android_thread_settings extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
    extern PFN_xrSetAndroidApplicationThreadKHR xrSetAndroidApplicationThreadKHR;
#endif // defined(XR_USE_PLATFORM_ANDROID)

    // ---- XR_KHR_android_surface_swapchain extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
    extern PFN_xrCreateSwapchainAndroidSurfaceKHR xrCreateSwapchainAndroidSurfaceKHR;
#endif // defined(XR_USE_PLATFORM_ANDROID)

    // ---- XR_KHR_opengl_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL)
    extern PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)

    // ---- XR_KHR_opengl_es_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    extern PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)

    // ---- XR_KHR_vulkan_enable extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensionsKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

    // ---- XR_KHR_D3D11_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D11)
    extern PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_D3D11)

    // ---- XR_KHR_D3D12_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D12)
    extern PFN_xrGetD3D12GraphicsRequirementsKHR xrGetD3D12GraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_D3D12)

    // ---- XR_KHR_visibility_mask extension commands
    extern PFN_xrGetVisibilityMaskKHR xrGetVisibilityMaskKHR;

    // ---- XR_KHR_win32_convert_performance_counter_time extension commands
#if defined(XR_USE_PLATFORM_WIN32)
    extern PFN_xrConvertWin32PerformanceCounterToTimeKHR xrConvertWin32PerformanceCounterToTimeKHR;
#endif // defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_PLATFORM_WIN32)
    extern PFN_xrConvertTimeToWin32PerformanceCounterKHR xrConvertTimeToWin32PerformanceCounterKHR;
#endif // defined(XR_USE_PLATFORM_WIN32)

    // ---- XR_KHR_convert_timespec_time extension commands
#if defined(XR_USE_TIMESPEC)
    extern PFN_xrConvertTimespecTimeToTimeKHR xrConvertTimespecTimeToTimeKHR;
#endif // defined(XR_USE_TIMESPEC)
#if defined(XR_USE_TIMESPEC)
    extern PFN_xrConvertTimeToTimespecTimeKHR xrConvertTimeToTimespecTimeKHR;
#endif // defined(XR_USE_TIMESPEC)

    // ---- XR_KHR_loader_init extension commands
    extern PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;

    // ---- XR_KHR_vulkan_enable2 extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrCreateVulkanInstanceKHR xrCreateVulkanInstanceKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrCreateVulkanDeviceKHR xrCreateVulkanDeviceKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrGetVulkanGraphicsDevice2KHR xrGetVulkanGraphicsDevice2KHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    extern PFN_xrGetVulkanGraphicsRequirements2KHR xrGetVulkanGraphicsRequirements2KHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
//};


// Prototype for dispatch table helper function
void PopulateOpenXRDispatchTable(XrInstance instance);

#ifdef __cplusplus
} // extern "C"
#endif

