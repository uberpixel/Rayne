//
//  RNVulkanInstance.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanInstance.h"
#include "RNVulkanDevice.h"
#include "RNVulkanDebug.h"

#if RN_PLATFORM_POSIX
#include <dlfcn.h>
#endif

namespace RN
{
	VulkanInstance::VulkanInstance(Array *instanceExtensions, Array *deviceExtensions) :
		_instance(nullptr),
		_module(nullptr),
		_allocationCallbacks(nullptr),
		_devices(nullptr)
	{
		_requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		_requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#if RN_PLATFORM_WINDOWS
		_requiredExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		_requiredDeviceExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
#endif
#if RN_PLATFORM_LINUX
		_requiredExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#if RN_PLATFORM_ANDROID
		_requiredExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif

#if RN_VULKAN_ENABLE_VALIDATION
		_requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

		_requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		_requiredDeviceExtensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME); //TODO: Required for instancing with crosscompiled shader, the features are enabled in the shaders but shouldn't actually be needed, enabling this makes it work for now

		if(instanceExtensions)
		{
			instanceExtensions->Enumerate<String>([&](String *extension, size_t index, bool &stop) {
				_requiredExtensions.push_back(extension->GetUTF8String());
			});
		}

		if(deviceExtensions)
		{
			deviceExtensions->Enumerate<String>([&](String *extension, size_t index, bool &stop) {
				_requiredDeviceExtensions.push_back(extension->GetUTF8String());
			});
		}
	}

	VulkanInstance::~VulkanInstance()
	{
		if(_instance)
			vk::DestroyInstance(_instance, _allocationCallbacks);

#if RN_PLATFORM_WINDOWS
		if(_module)
			::FreeModule(_module);
#endif

		SafeRelease(_devices);
	}

	bool VulkanInstance::LoadVulkan()
	{
		if(_instance)
			return true;

#if RN_PLATFORM_WINDOWS
		if(!_module)
		{
			// Load the module and verify that we have vkGetInstanceProcAddr() available
			_module = ::LoadLibrary("vulkan-1.dll");
		}

		PFN_vkGetInstanceProcAddr procAddr = nullptr;

		if(_module)
			procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(::GetProcAddress(_module, "vkGetInstanceProcAddr"));

		if(!_module || !procAddr)
		{
			RNError("Couldn't load Vulkan library");

			if(_module)
			{
				::FreeLibrary(_module);
				_module = nullptr;
			}

			return false;
		}
#endif

#if RN_PLATFORM_POSIX
		if(!_module)
		{
#if RN_PLATFORM_ANDROID
			_module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
#else
			//_module = dlopen("/usr/lib/x86_64-linux-gnu/libvulkan.so.1", RTLD_NOW | RTLD_GLOBAL);
			_module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
			if(!_module)
			{
				RNDebug("fuck: " << dlerror());
			}
#endif
		}

		PFN_vkGetInstanceProcAddr procAddr = nullptr;

		if(_module)
			procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(_module, "vkGetInstanceProcAddr"));

		if(!_module || !procAddr)
		{
			RNError("Couldn't load Vulkan library");

			if(_module)
			{
				dlclose(_module);
				_module = nullptr;
			}

			return false;
		}
#endif

		vk::init_dispatch_table_top(procAddr);

		// TODO: Verify extensions
		std::vector<const char *> layers = DebugInstanceLayers();

#if RN_BUILD_DEBUG
		std::vector<VkExtensionProperties> rawInstanceExtensions;
		EnumerateExtensions(nullptr, rawInstanceExtensions);

		for(const auto &extension : rawInstanceExtensions)
		{
			RNDebug("Supported Vulkan Instance Extension: " << extension.extensionName);
		}
#endif

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = Kernel::GetSharedInstance()->GetApplication()->GetTitle()->GetUTF8String();
		appInfo.applicationVersion = 0;
		appInfo.pEngineName = "Rayne";
		appInfo.engineVersion = GetAPIVersion();
		appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

		VkInstanceCreateInfo instanceInfo = {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(_requiredExtensions.size());
		instanceInfo.ppEnabledExtensionNames = _requiredExtensions.data();
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		instanceInfo.ppEnabledLayerNames = layers.data();

		VkResult result = vk::CreateInstance(&instanceInfo, _allocationCallbacks, &_instance);
		if(result != VK_SUCCESS)
		{
			RNDumpVulkanResult(result);
			_instance = nullptr;

			return false;
		}

		vk::init_dispatch_table_middle(_instance, false);

		SetupVulkanDebugging(_instance);

		// Load the list of devices
		{
			_devices = new Array();

			std::vector<VkPhysicalDevice> devices;
			if(EnumerateDevicesWithExtensions(devices, _requiredDeviceExtensions) == VK_SUCCESS)
			{
				for(auto device : devices)
				{
					VulkanDevice *temp = new VulkanDevice(this, device);
					if(temp->IsValidDevice())
					{
						_devices->AddObject(temp);
					}

					temp->Release();
				}
			}
		}

#if RN_BUILD_DEBUG
		std::vector<VkPhysicalDevice> devices;
		EnumerateDevices(devices);

		for(auto device : devices)
		{
			std::vector<VkExtensionProperties> rawDeviceExtensions;
			EnumerateDeviceExtensions(device, nullptr, rawDeviceExtensions);

			for(const auto &extension : rawDeviceExtensions)
				RNDebug("Supported Vulkan Device Extension: " << extension.extensionName);
		}
#endif

		return true;
	}

	VkResult VulkanInstance::EnumerateExtensions(const char *layer, std::vector<VkExtensionProperties> &extensions) const
	{
		uint32_t count = 0;
		RNVulkanValidate(vk::EnumerateInstanceExtensionProperties(layer, &count, nullptr));

		extensions.resize(count);
		return vk::EnumerateInstanceExtensionProperties(layer, &count, extensions.data());
	}

	VkResult VulkanInstance::EnumerateDeviceExtensions(VkPhysicalDevice device, const char *layer, std::vector<VkExtensionProperties> &extensions) const
	{
		uint32_t count = 0;
		RNVulkanValidate(vk::EnumerateDeviceExtensionProperties(device, layer, &count, nullptr));

		extensions.resize(count);
		return vk::EnumerateDeviceExtensionProperties(device, layer, &count, extensions.data());
	}



	VkResult VulkanInstance::EnumerateDevices(std::vector<VkPhysicalDevice> &devices) const
	{
		uint32_t count = 0;
		RNVulkanValidate(vk::EnumeratePhysicalDevices(_instance, &count, nullptr));

		devices.resize(count);
		return vk::EnumeratePhysicalDevices(_instance, &count, devices.data());
	}

	VkResult VulkanInstance::EnumerateDevicesWithExtensions(std::vector<VkPhysicalDevice> &devices, const std::vector<const char *> &extensions) const
	{
		VkResult result = EnumerateDevices(devices);
		if(result == VK_SUCCESS)
		{
			for(auto iterator = devices.begin(); iterator != devices.end();)
			{
				if(!DeviceSupportsExtensions(*iterator, extensions))
				{
					iterator = devices.erase(iterator);
					continue;
				}

				iterator ++;
			}
		}

		return result;
	}

	bool VulkanInstance::DeviceSupportsExtensions(VkPhysicalDevice device, const std::vector<const char *> &extensions) const
	{
		std::vector<VkExtensionProperties> rawDeviceExtensions;
		EnumerateDeviceExtensions(device, nullptr, rawDeviceExtensions);

		std::unordered_set<std::string> deviceExtensions;
		for (const auto &extension : rawDeviceExtensions)
			deviceExtensions.insert(extension.extensionName);

		for(const auto &name : extensions)
		{
			if(deviceExtensions.find(name) == deviceExtensions.end())
				return false;
		}

		return true;
	}


	String *GetStringForVulkanResult(VkResult result)
	{
		switch(result)
		{
			case VK_SUCCESS:
				return RNCSTR("VK_SUCCESS");
			case VK_NOT_READY:
				return RNCSTR("VK_NOT_READY");
			case VK_TIMEOUT:
				return RNCSTR("VK_TIMEOUT");
			case VK_EVENT_SET:
				return RNCSTR("VK_EVENT_SET");
			case VK_EVENT_RESET:
				return RNCSTR("VK_EVENT_RESET");
			case VK_INCOMPLETE:
				return RNCSTR("VK_INCOMPLETE");

			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return RNCSTR("VK_ERROR_OUT_OF_HOST_MEMORY");
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return RNCSTR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			case VK_ERROR_INITIALIZATION_FAILED:
				return RNCSTR("VK_ERROR_INITIALIZATION_FAILED");
			case VK_ERROR_DEVICE_LOST:
				return RNCSTR("VK_ERROR_DEVICE_LOST");
			case VK_ERROR_MEMORY_MAP_FAILED:
				return RNCSTR("VK_ERROR_MEMORY_MAP_FAILED");
			case VK_ERROR_LAYER_NOT_PRESENT:
				return RNCSTR("VK_ERROR_LAYER_NOT_PRESENT");
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				return RNCSTR("VK_ERROR_EXTENSION_NOT_PRESENT");
			case VK_ERROR_FEATURE_NOT_PRESENT:
				return RNCSTR("VK_ERROR_FEATURE_NOT_PRESENT");
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				return RNCSTR("VK_ERROR_INCOMPATIBLE_DRIVER");
			case VK_ERROR_TOO_MANY_OBJECTS:
				return RNCSTR("VK_ERROR_TOO_MANY_OBJECTS");
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				return RNCSTR("VK_ERROR_FORMAT_NOT_SUPPORTED");

			default:
				return RNSTR("Unknown error (" << result << ")");
		}
	}
}
