//
//  RNVulkanDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANDEVICE_H_
#define __RAYNE_VULKANDEVICE_H_

#include <Rayne.h>
#include "RNVulkanDispatchTable.h"

namespace RN
{
	class VulkanIntance;
	class VulkanDevice : public Object
	{
	public:
		friend class VulkanInstance;

		const String *GetDescription() const override;

		const String *GetName() const { return _name; }

		uint32 GetAPIVersion() const { return _apiVersion; }
		uint32 GetDriverVersion() const { return _driverVersion; }
		uint32 GetVendorID() const { return _vendorID; }

	private:
		VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device);

		bool ParseDevice();

		VulkanInstance *_instance;
		VkPhysicalDevice _device;

		String *_name;

		uint32_t _apiVersion;
		uint32_t _driverVersion;
		uint32_t _vendorID;
		uint32_t _deviceID;

		VkPhysicalDeviceType _type;

		RNDeclareMeta(VulkanDevice)
	};
}


#endif /* __RAYNE_VULKANDEVICE_H_ */
