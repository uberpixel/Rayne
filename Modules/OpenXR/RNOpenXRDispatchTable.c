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

#include "RNOpenXRDispatchTable.h"

#ifdef __cplusplus
extern "C" { 
#endif

PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr;
PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
PFN_xrCreateInstance xrCreateInstance;
PFN_xrDestroyInstance xrDestroyInstance;
PFN_xrGetInstanceProperties xrGetInstanceProperties;
PFN_xrPollEvent xrPollEvent;
PFN_xrResultToString xrResultToString;
PFN_xrStructureTypeToString xrStructureTypeToString;
PFN_xrGetSystem xrGetSystem;
PFN_xrGetSystemProperties xrGetSystemProperties;
PFN_xrEnumerateEnvironmentBlendModes xrEnumerateEnvironmentBlendModes;
PFN_xrCreateSession xrCreateSession;
PFN_xrDestroySession xrDestroySession;
PFN_xrEnumerateReferenceSpaces xrEnumerateReferenceSpaces;
PFN_xrCreateReferenceSpace xrCreateReferenceSpace;
PFN_xrGetReferenceSpaceBoundsRect xrGetReferenceSpaceBoundsRect;
PFN_xrCreateActionSpace xrCreateActionSpace;
PFN_xrLocateSpace xrLocateSpace;
PFN_xrDestroySpace xrDestroySpace;
PFN_xrEnumerateViewConfigurations xrEnumerateViewConfigurations;
PFN_xrGetViewConfigurationProperties xrGetViewConfigurationProperties;
PFN_xrEnumerateViewConfigurationViews xrEnumerateViewConfigurationViews;
PFN_xrEnumerateSwapchainFormats xrEnumerateSwapchainFormats;
PFN_xrCreateSwapchain xrCreateSwapchain;
PFN_xrDestroySwapchain xrDestroySwapchain;
PFN_xrEnumerateSwapchainImages xrEnumerateSwapchainImages;
PFN_xrAcquireSwapchainImage xrAcquireSwapchainImage;
PFN_xrWaitSwapchainImage xrWaitSwapchainImage;
PFN_xrReleaseSwapchainImage xrReleaseSwapchainImage;
PFN_xrBeginSession xrBeginSession;
PFN_xrEndSession xrEndSession;
PFN_xrRequestExitSession xrRequestExitSession;
PFN_xrWaitFrame xrWaitFrame;
PFN_xrBeginFrame xrBeginFrame;
PFN_xrEndFrame xrEndFrame;
PFN_xrLocateViews xrLocateViews;
PFN_xrStringToPath xrStringToPath;
PFN_xrPathToString xrPathToString;
PFN_xrCreateActionSet xrCreateActionSet;
PFN_xrDestroyActionSet xrDestroyActionSet;
PFN_xrCreateAction xrCreateAction;
PFN_xrDestroyAction xrDestroyAction;
PFN_xrSuggestInteractionProfileBindings xrSuggestInteractionProfileBindings;
PFN_xrAttachSessionActionSets xrAttachSessionActionSets;
PFN_xrGetCurrentInteractionProfile xrGetCurrentInteractionProfile;
PFN_xrGetActionStateBoolean xrGetActionStateBoolean;
PFN_xrGetActionStateFloat xrGetActionStateFloat;
PFN_xrGetActionStateVector2f xrGetActionStateVector2f;
PFN_xrGetActionStatePose xrGetActionStatePose;
PFN_xrSyncActions xrSyncActions;
PFN_xrEnumerateBoundSourcesForAction xrEnumerateBoundSourcesForAction;
PFN_xrGetInputSourceLocalizedName xrGetInputSourceLocalizedName;
PFN_xrApplyHapticFeedback xrApplyHapticFeedback;
PFN_xrStopHapticFeedback xrStopHapticFeedback;

    // ---- XR_KHR_android_thread_settings extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
    PFN_xrSetAndroidApplicationThreadKHR xrSetAndroidApplicationThreadKHR;
#endif // defined(XR_USE_PLATFORM_ANDROID)

    // ---- XR_KHR_android_surface_swapchain extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
    PFN_xrCreateSwapchainAndroidSurfaceKHR xrCreateSwapchainAndroidSurfaceKHR;
#endif // defined(XR_USE_PLATFORM_ANDROID)

    // ---- XR_KHR_opengl_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL)
    PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)

    // ---- XR_KHR_opengl_es_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    PFN_xrGetOpenGLESGraphicsRequirementsKHR xrGetOpenGLESGraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)

    // ---- XR_KHR_vulkan_enable extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensionsKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

    // ---- XR_KHR_D3D11_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D11)
    PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_D3D11)

    // ---- XR_KHR_D3D12_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D12)
    PFN_xrGetD3D12GraphicsRequirementsKHR xrGetD3D12GraphicsRequirementsKHR;
#endif // defined(XR_USE_GRAPHICS_API_D3D12)

    // ---- XR_KHR_visibility_mask extension commands
    PFN_xrGetVisibilityMaskKHR xrGetVisibilityMaskKHR;

    // ---- XR_KHR_win32_convert_performance_counter_time extension commands
#if defined(XR_USE_PLATFORM_WIN32)
    PFN_xrConvertWin32PerformanceCounterToTimeKHR xrConvertWin32PerformanceCounterToTimeKHR;
#endif // defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_PLATFORM_WIN32)
    PFN_xrConvertTimeToWin32PerformanceCounterKHR xrConvertTimeToWin32PerformanceCounterKHR;
#endif // defined(XR_USE_PLATFORM_WIN32)

    // ---- XR_KHR_convert_timespec_time extension commands
#if defined(XR_USE_TIMESPEC)
    PFN_xrConvertTimespecTimeToTimeKHR xrConvertTimespecTimeToTimeKHR;
#endif // defined(XR_USE_TIMESPEC)
#if defined(XR_USE_TIMESPEC)
    PFN_xrConvertTimeToTimespecTimeKHR xrConvertTimeToTimespecTimeKHR;
#endif // defined(XR_USE_TIMESPEC)

    // ---- XR_KHR_loader_init extension commands
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;

    // ---- XR_KHR_vulkan_enable2 extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrCreateVulkanInstanceKHR xrCreateVulkanInstanceKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrCreateVulkanDeviceKHR xrCreateVulkanDeviceKHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrGetVulkanGraphicsDevice2KHR xrGetVulkanGraphicsDevice2KHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    PFN_xrGetVulkanGraphicsRequirements2KHR xrGetVulkanGraphicsRequirements2KHR;
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)


// Helper function to populate an instance dispatch table
void PopulateOpenXRDispatchTable(XrInstance instance) {

    // ---- Core 1.0 commands
    (xrGetInstanceProcAddr(instance, "xrEnumerateApiLayerProperties", (PFN_xrVoidFunction*)&xrEnumerateApiLayerProperties));
    (xrGetInstanceProcAddr(instance, "xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionProperties));
    
    (xrGetInstanceProcAddr(instance, "xrCreateInstance", (PFN_xrVoidFunction*)&xrCreateInstance));
    (xrGetInstanceProcAddr(instance, "xrDestroyInstance", (PFN_xrVoidFunction*)&xrDestroyInstance));
    (xrGetInstanceProcAddr(instance, "xrGetInstanceProperties", (PFN_xrVoidFunction*)&xrGetInstanceProperties));
    (xrGetInstanceProcAddr(instance, "xrPollEvent", (PFN_xrVoidFunction*)&xrPollEvent));
    (xrGetInstanceProcAddr(instance, "xrResultToString", (PFN_xrVoidFunction*)&xrResultToString));
    (xrGetInstanceProcAddr(instance, "xrStructureTypeToString", (PFN_xrVoidFunction*)&xrStructureTypeToString));
    (xrGetInstanceProcAddr(instance, "xrGetSystem", (PFN_xrVoidFunction*)&xrGetSystem));
    (xrGetInstanceProcAddr(instance, "xrGetSystemProperties", (PFN_xrVoidFunction*)&xrGetSystemProperties));
    (xrGetInstanceProcAddr(instance, "xrEnumerateEnvironmentBlendModes", (PFN_xrVoidFunction*)&xrEnumerateEnvironmentBlendModes));
    (xrGetInstanceProcAddr(instance, "xrCreateSession", (PFN_xrVoidFunction*)&xrCreateSession));
    (xrGetInstanceProcAddr(instance, "xrDestroySession", (PFN_xrVoidFunction*)&xrDestroySession));
    (xrGetInstanceProcAddr(instance, "xrEnumerateReferenceSpaces", (PFN_xrVoidFunction*)&xrEnumerateReferenceSpaces));
    (xrGetInstanceProcAddr(instance, "xrCreateReferenceSpace", (PFN_xrVoidFunction*)&xrCreateReferenceSpace));
    (xrGetInstanceProcAddr(instance, "xrGetReferenceSpaceBoundsRect", (PFN_xrVoidFunction*)&xrGetReferenceSpaceBoundsRect));
    (xrGetInstanceProcAddr(instance, "xrCreateActionSpace", (PFN_xrVoidFunction*)&xrCreateActionSpace));
    (xrGetInstanceProcAddr(instance, "xrLocateSpace", (PFN_xrVoidFunction*)&xrLocateSpace));
    (xrGetInstanceProcAddr(instance, "xrDestroySpace", (PFN_xrVoidFunction*)&xrDestroySpace));
    (xrGetInstanceProcAddr(instance, "xrEnumerateViewConfigurations", (PFN_xrVoidFunction*)&xrEnumerateViewConfigurations));
    (xrGetInstanceProcAddr(instance, "xrGetViewConfigurationProperties", (PFN_xrVoidFunction*)&xrGetViewConfigurationProperties));
    (xrGetInstanceProcAddr(instance, "xrEnumerateViewConfigurationViews", (PFN_xrVoidFunction*)&xrEnumerateViewConfigurationViews));
    (xrGetInstanceProcAddr(instance, "xrEnumerateSwapchainFormats", (PFN_xrVoidFunction*)&xrEnumerateSwapchainFormats));
    (xrGetInstanceProcAddr(instance, "xrCreateSwapchain", (PFN_xrVoidFunction*)&xrCreateSwapchain));
    (xrGetInstanceProcAddr(instance, "xrDestroySwapchain", (PFN_xrVoidFunction*)&xrDestroySwapchain));
    (xrGetInstanceProcAddr(instance, "xrEnumerateSwapchainImages", (PFN_xrVoidFunction*)&xrEnumerateSwapchainImages));
    (xrGetInstanceProcAddr(instance, "xrAcquireSwapchainImage", (PFN_xrVoidFunction*)&xrAcquireSwapchainImage));
    (xrGetInstanceProcAddr(instance, "xrWaitSwapchainImage", (PFN_xrVoidFunction*)&xrWaitSwapchainImage));
    (xrGetInstanceProcAddr(instance, "xrReleaseSwapchainImage", (PFN_xrVoidFunction*)&xrReleaseSwapchainImage));
    (xrGetInstanceProcAddr(instance, "xrBeginSession", (PFN_xrVoidFunction*)&xrBeginSession));
    (xrGetInstanceProcAddr(instance, "xrEndSession", (PFN_xrVoidFunction*)&xrEndSession));
    (xrGetInstanceProcAddr(instance, "xrRequestExitSession", (PFN_xrVoidFunction*)&xrRequestExitSession));
    (xrGetInstanceProcAddr(instance, "xrWaitFrame", (PFN_xrVoidFunction*)&xrWaitFrame));
    (xrGetInstanceProcAddr(instance, "xrBeginFrame", (PFN_xrVoidFunction*)&xrBeginFrame));
    (xrGetInstanceProcAddr(instance, "xrEndFrame", (PFN_xrVoidFunction*)&xrEndFrame));
    (xrGetInstanceProcAddr(instance, "xrLocateViews", (PFN_xrVoidFunction*)&xrLocateViews));
    (xrGetInstanceProcAddr(instance, "xrStringToPath", (PFN_xrVoidFunction*)&xrStringToPath));
    (xrGetInstanceProcAddr(instance, "xrPathToString", (PFN_xrVoidFunction*)&xrPathToString));
    (xrGetInstanceProcAddr(instance, "xrCreateActionSet", (PFN_xrVoidFunction*)&xrCreateActionSet));
    (xrGetInstanceProcAddr(instance, "xrDestroyActionSet", (PFN_xrVoidFunction*)&xrDestroyActionSet));
    (xrGetInstanceProcAddr(instance, "xrCreateAction", (PFN_xrVoidFunction*)&xrCreateAction));
    (xrGetInstanceProcAddr(instance, "xrDestroyAction", (PFN_xrVoidFunction*)&xrDestroyAction));
    (xrGetInstanceProcAddr(instance, "xrSuggestInteractionProfileBindings", (PFN_xrVoidFunction*)&xrSuggestInteractionProfileBindings));
    (xrGetInstanceProcAddr(instance, "xrAttachSessionActionSets", (PFN_xrVoidFunction*)&xrAttachSessionActionSets));
    (xrGetInstanceProcAddr(instance, "xrGetCurrentInteractionProfile", (PFN_xrVoidFunction*)&xrGetCurrentInteractionProfile));
    (xrGetInstanceProcAddr(instance, "xrGetActionStateBoolean", (PFN_xrVoidFunction*)&xrGetActionStateBoolean));
    (xrGetInstanceProcAddr(instance, "xrGetActionStateFloat", (PFN_xrVoidFunction*)&xrGetActionStateFloat));
    (xrGetInstanceProcAddr(instance, "xrGetActionStateVector2f", (PFN_xrVoidFunction*)&xrGetActionStateVector2f));
    (xrGetInstanceProcAddr(instance, "xrGetActionStatePose", (PFN_xrVoidFunction*)&xrGetActionStatePose));
    (xrGetInstanceProcAddr(instance, "xrSyncActions", (PFN_xrVoidFunction*)&xrSyncActions));
    (xrGetInstanceProcAddr(instance, "xrEnumerateBoundSourcesForAction", (PFN_xrVoidFunction*)&xrEnumerateBoundSourcesForAction));
    (xrGetInstanceProcAddr(instance, "xrGetInputSourceLocalizedName", (PFN_xrVoidFunction*)&xrGetInputSourceLocalizedName));
    (xrGetInstanceProcAddr(instance, "xrApplyHapticFeedback", (PFN_xrVoidFunction*)&xrApplyHapticFeedback));
    (xrGetInstanceProcAddr(instance, "xrStopHapticFeedback", (PFN_xrVoidFunction*)&xrStopHapticFeedback));

    // ---- XR_KHR_android_thread_settings extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
    (xrGetInstanceProcAddr(instance, "xrSetAndroidApplicationThreadKHR", (PFN_xrVoidFunction*)&xrSetAndroidApplicationThreadKHR));
#endif // defined(XR_USE_PLATFORM_ANDROID)

    // ---- XR_KHR_android_surface_swapchain extension commands
#if defined(XR_USE_PLATFORM_ANDROID)
    (xrGetInstanceProcAddr(instance, "xrCreateSwapchainAndroidSurfaceKHR", (PFN_xrVoidFunction*)&xrCreateSwapchainAndroidSurfaceKHR));
#endif // defined(XR_USE_PLATFORM_ANDROID)

    // ---- XR_KHR_opengl_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL)
    (xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetOpenGLGraphicsRequirementsKHR));
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)

    // ---- XR_KHR_opengl_es_enable extension commands
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
    (xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetOpenGLESGraphicsRequirementsKHR));
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)

    // ---- XR_KHR_vulkan_enable extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction*)&xrGetVulkanInstanceExtensionsKHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction*)&xrGetVulkanDeviceExtensionsKHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsDeviceKHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsRequirementsKHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)

    // ---- XR_KHR_D3D11_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D11)
    (xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetD3D11GraphicsRequirementsKHR));
#endif // defined(XR_USE_GRAPHICS_API_D3D11)

    // ---- XR_KHR_D3D12_enable extension commands
#if defined(XR_USE_GRAPHICS_API_D3D12)
    (xrGetInstanceProcAddr(instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&xrGetD3D12GraphicsRequirementsKHR));
#endif // defined(XR_USE_GRAPHICS_API_D3D12)

    // ---- XR_KHR_visibility_mask extension commands
    (xrGetInstanceProcAddr(instance, "xrGetVisibilityMaskKHR", (PFN_xrVoidFunction*)&xrGetVisibilityMaskKHR));

    // ---- XR_KHR_win32_convert_performance_counter_time extension commands
#if defined(XR_USE_PLATFORM_WIN32)
    (xrGetInstanceProcAddr(instance, "xrConvertWin32PerformanceCounterToTimeKHR", (PFN_xrVoidFunction*)&xrConvertWin32PerformanceCounterToTimeKHR));
#endif // defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_PLATFORM_WIN32)
    (xrGetInstanceProcAddr(instance, "xrConvertTimeToWin32PerformanceCounterKHR", (PFN_xrVoidFunction*)&xrConvertTimeToWin32PerformanceCounterKHR));
#endif // defined(XR_USE_PLATFORM_WIN32)

    // ---- XR_KHR_convert_timespec_time extension commands
#if defined(XR_USE_TIMESPEC)
    (xrGetInstanceProcAddr(instance, "xrConvertTimespecTimeToTimeKHR", (PFN_xrVoidFunction*)&xrConvertTimespecTimeToTimeKHR));
#endif // defined(XR_USE_TIMESPEC)
#if defined(XR_USE_TIMESPEC)
    (xrGetInstanceProcAddr(instance, "xrConvertTimeToTimespecTimeKHR", (PFN_xrVoidFunction*)&xrConvertTimeToTimespecTimeKHR));
#endif // defined(XR_USE_TIMESPEC)

    // ---- XR_KHR_vulkan_enable2 extension commands
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR", (PFN_xrVoidFunction*)&xrCreateVulkanInstanceKHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR", (PFN_xrVoidFunction*)&xrCreateVulkanDeviceKHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsDevice2KHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    (xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&xrGetVulkanGraphicsRequirements2KHR));
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
}


#ifdef __cplusplus
} // extern "C"
#endif

