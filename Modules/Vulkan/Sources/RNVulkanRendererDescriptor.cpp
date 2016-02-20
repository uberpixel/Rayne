//
//  RNVulkanRendererDescriptor.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanRendererDescriptor.h"
#include "RNVulkanDispatchTable.h"

namespace RN
{
	RNDefineMeta(VulkanRendererDescriptor, RendererDescriptor)

	static VulkanInstance *__vulkanInstance = nullptr;

	void VulkanRendererDescriptor::InitialWakeUp(MetaClass *meta)
	{
		if(meta == VulkanRendererDescriptor::GetMetaClass())
		{
			VulkanRendererDescriptor *descriptor = new VulkanRendererDescriptor();
			RendererManager::GetSharedInstance()->AddDescriptor(descriptor);
			descriptor->Release();
		}
	}

	VulkanRendererDescriptor::VulkanRendererDescriptor() :
		RN::RendererDescriptor(RNCSTR("net.uberpixel.rendering.vulkan"), RNCSTR("Vulkan"))
	{}

	Renderer *VulkanRendererDescriptor::CreateRenderer(const Dictionary *parameters)
	{
		return nullptr;
	}

	bool VulkanRendererDescriptor::CanConstructWithSettings(const Dictionary *parameters) const
	{
		if(!__vulkanInstance)
			__vulkanInstance = new VulkanInstance();


		if(__vulkanInstance->LoadVulkan())
		{
			Array *devices = __vulkanInstance->GetDevices();
			devices->Enumerate<VulkanDevice>([&](VulkanDevice *device, size_t index, bool &stop) {

				RNInfo(device);

			});
		}

		return false;
	}
}
