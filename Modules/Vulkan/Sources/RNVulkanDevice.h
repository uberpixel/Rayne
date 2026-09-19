//
//  RNVulkanDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANDEVICE_H_
#define __RAYNE_VULKANDEVICE_H_

#include "RNVulkan.h"
#include "RNVulkanDispatchTable.h"
#include "RNVulkanInstance.h"

namespace RN
{
	class VulkanGraphicsProvider;
	class VulkanDevice : public RenderingDevice
	{
	public:
		friend class VulkanInstance;

		~VulkanDevice();

		VKAPI bool CreateDevice(const std::vector<const char *> &extensions, VulkanGraphicsProvider *graphicsProvider = nullptr);
		bool IsValidDevice() const { return (_workQueue != kRNNotFound); }

		VKAPI void GetQueueProperties(std::vector<VkQueueFamilyProperties> &queues);
		VKAPI VkResult GetSurfaceFormats(VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &formats);
		VKAPI VkResult GetPresentModes(VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes);

		VkDevice GetDevice() const { return _device; }
		VkPhysicalDevice GetPhysicalDevice() const { return _physicalDevice; }
		VulkanInstance *GetInstance() const { return _instance; }

		void GetPipelineCacheUUID(uint8 *uuid);

		uint32_t GetWorkQueue() const { return static_cast<uint32_t>(_workQueue); }

		VKAPI VkBool32 GetMemoryWithType(uint32_t typeBits, VkFlags properties, uint32_t &typeIndex) const;
		VKAPI size_t GetMemoryWithType(VkMemoryPropertyFlagBits required) const;

		bool GetSupportsMultiview() const { return _maxMultiviewViewCount > 1; }
		uint32 GetMaxMultiviewViewCount() const { return _maxMultiviewViewCount; }
		bool GetSupportsFragmentDensityMaps() const { return _supportsFragmentDensityMaps; }
		bool GetSupportsFragmentDensityMaps2() const { return _supportsFragmentDensityMaps2; }
		bool GetSupportsSamplerAnisotropy() const { return _supportsSamplerAnisotropy; }
		float GetMaxSamplerAnisotropy() const { return _maxSamplerAnisotropy; }
		bool GetSupportsFullscreenExclusive() const { return _supportsFullscreenExclusive; }
		bool GetSupportsTileProperties() const { return _supportsTileProperties; }
		bool GetSupportsMultiDrawIndirect() const { return _supportsMultiDrawIndirect; }
		bool GetSupportsShaderFloat16() const { return _supportsShaderFloat16; }
		bool GetSupportsShaderInt8() const { return _supportsShaderInt8; }
		bool GetSupportsExternalTextureImport() const { return _supportsExternalTextureImport; }
		bool GetSupportsExternalTextureSynchronization() const { return _supportsExternalTextureSynchronization; }

		VKAPI bool GetDeviceLUID(uint8 *luid) const;

		VKAPI void SetExtensions(Array *extensions) final;

	private:
		VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device);

		static Descriptor DescriptorForDevice(VkPhysicalDevice device);
		static bool AddDeviceExtensionIfAvailable(std::vector<const char *> &enabledExtensions, const std::vector<VkExtensionProperties> &availableExtensions, const char *name);

		RN::Array *_deviceExtensions;

		uint32 _maxMultiviewViewCount;
		bool _supportsFragmentDensityMaps;
		Vector2 _minFragmentDensityTexelSize;
		Vector2 _maxFragmentDensityTexelSize;
		bool _supportsFragmentDensityMaps2;
		bool _hasFragmentDensitySubsampledLoads;
		bool _hasFragmentDensitySubsampledCoarseReconstructionEarlyAccess;
		uint32 _maxFragmentDensitySubsampledLayers;
		uint32 _maxFragmentDensitySubsampledSamplers;
		bool _supportsSamplerAnisotropy;
		bool _supportsFullscreenExclusive;
		bool _supportsMultiDrawIndirect;
		bool _supportsShaderFloat16;
		bool _supportsShaderInt8;
		float _maxSamplerAnisotropy;
		bool _supportsTileProperties;
		bool _supportsExternalTextureImport;
		bool _supportsExternalTextureSynchronization;

		bool _hasDeviceLUID;
		uint8 _deviceLUID[VK_LUID_SIZE];

		VulkanInstance *_instance;
		VkPhysicalDevice _physicalDevice;
		VkDevice _device;
		uint8 pipelineCacheUUID[VK_UUID_SIZE];

		size_t _workQueue;

		VkPhysicalDeviceMemoryProperties _memoryProperties;

		RNDeclareMetaAPI(VulkanDevice, VKAPI)
	};
}


#endif /* __RAYNE_VULKANDEVICE_H_ */
