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

	Renderer *VulkanRendererDescriptor::CreateRenderer(RenderingDevice *device)
	{
		return nullptr;
	}

	void VulkanRendererDescriptor::PrepareWithSettings(const Dictionary *settings)
	{
		_instance = new VulkanInstance();
		if(!_instance->LoadVulkan())
		{
			delete _instance;
			_instance = nullptr;
		}
	}

	bool VulkanRendererDescriptor::CanCreateRenderer() const
	{
		return (_instance && _instance->GetDevices()->GetCount() > 0);
	}
}
