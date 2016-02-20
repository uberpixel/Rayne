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
	class VulkanDevice : public RenderingDevice
	{
	public:
		friend class VulkanInstance;

	private:
		VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device);

		static Descriptor DescriptorForDevice(VkPhysicalDevice device);

		VulkanInstance *_instance;
		VkPhysicalDevice _device;

		RNDeclareMeta(VulkanDevice)
	};
}


#endif /* __RAYNE_VULKANDEVICE_H_ */
