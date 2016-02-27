//
//  RNVulkanDebug.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanDebug.h"

namespace vk
{
	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
	PFN_vkDebugReportMessageEXT DebugReportMessage;
}

namespace RN
{
	static VkDebugReportCallbackEXT __debugReportCallback;

	VkBool32 DebugMessageCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData)
	{
		if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			RNError("[" << pLayerPrefix << "] (" << (void *)srcObject << ") Code " << msgCode << ": " << pMsg);
			return VK_TRUE;
		}
		else if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			RNWarning("[" << pLayerPrefix << "] (" << (void *)srcObject <<  ") Code " << msgCode << ": " << pMsg);
			return VK_TRUE;
		}
		else if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		{
			RNDebug("[" << pLayerPrefix << "] (" << (void *)srcObject << ") Code " << msgCode << ": " << pMsg);
			return VK_TRUE;
		}
		else if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		{
			RNWarning("(Perf) [" << pLayerPrefix << "] (" << (void *)srcObject << ") Code " << msgCode << ": " << pMsg);
			return VK_TRUE;
		}
		else if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		{
			RNInfo("[" << pLayerPrefix << "] (" << (void *)srcObject << ") Code " << msgCode << ": " << pMsg);
			return VK_TRUE;
		}

		return VK_FALSE;
	}

	void VulkanErrorBreak(VkResult result)
	{
		RNError("Encountered Vulkan error " <<  result << ", put a breakpoint in RN::VulkanErrorBreak() to catch");
	}

	bool SetupVulkanDebugging(VkInstance instance)
	{
		vk::CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vk::GetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		vk::DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vk::GetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		vk::DebugReportMessage = (PFN_vkDebugReportMessageEXT)vk::GetInstanceProcAddr(instance, "vkDebugReportMessageEXT");

		if(!vk::CreateDebugReportCallback || !vk::DestroyDebugReportCallback || !vk::DebugReportMessage)
			return false;

		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.pNext = nullptr;
		dbgCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)DebugMessageCallback;
		dbgCreateInfo.pUserData = nullptr;
		dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

		VkResult result = vk::CreateDebugReportCallback(instance, &dbgCreateInfo, nullptr, &__debugReportCallback);
		RNVulkanValidate(result);

		return (result == VK_SUCCESS);
	}

	std::vector<const char *> DebugInstanceLayers()
	{
		std::vector<const char *> layers;

#if RN_VULKAN_ENABLE_VALIDATION
		//layers.push_back("VK_LAYER_RENDERDOC_Capture");
		//layers.push_back("VK_LAYER_LUNARG_api_dump");

		//layers.push_back("VK_LAYER_LUNARG_mem_tracker");
		layers.push_back("VK_LAYER_LUNARG_threading");
		layers.push_back("VK_LAYER_LUNARG_object_tracker");
		layers.push_back("VK_LAYER_LUNARG_draw_state");
		layers.push_back("VK_LAYER_LUNARG_param_checker");
		layers.push_back("VK_LAYER_LUNARG_swapchain");
		layers.push_back("VK_LAYER_LUNARG_device_limits");
		layers.push_back("VK_LAYER_LUNARG_image");
		layers.push_back("VK_LAYER_GOOGLE_unique_objects");
#endif

		return layers;
	}

	std::vector<const char *> DebugDeviceLayers()
	{
		std::vector<const char *> layers;

#if RN_VULKAN_ENABLE_VALIDATION
		//layers.push_back("VK_LAYER_RENDERDOC_Capture");
		//layers.push_back("VK_LAYER_LUNARG_api_dump");

		//layers.push_back("VK_LAYER_LUNARG_mem_tracker");
		layers.push_back("VK_LAYER_LUNARG_threading");
		layers.push_back("VK_LAYER_LUNARG_object_tracker");
		layers.push_back("VK_LAYER_LUNARG_draw_state");
		layers.push_back("VK_LAYER_LUNARG_param_checker");
		layers.push_back("VK_LAYER_LUNARG_swapchain");
		layers.push_back("VK_LAYER_LUNARG_device_limits");
		layers.push_back("VK_LAYER_LUNARG_image");
		layers.push_back("VK_LAYER_GOOGLE_unique_objects");
#endif

		return layers;
	}
}
