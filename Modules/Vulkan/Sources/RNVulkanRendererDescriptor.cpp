//
//  RNVulkanRendererDescriptor.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanRendererDescriptor.h"
#include "RNVulkanDispatchTable.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanDevice.h"

namespace RN
{
	RNDefineMeta(VulkanRendererDescriptor, RendererDescriptor)

	void VulkanRendererDescriptor::InitialWakeUp(MetaClass *meta)
	{
		if(meta == VulkanRendererDescriptor::GetMetaClass())
		{
			VulkanRendererDescriptor *descriptor = new VulkanRendererDescriptor();
			GetExtensionPoint()->AddExtension(descriptor, 0);
			descriptor->Release();
		}
	}

	VulkanRendererDescriptor::VulkanRendererDescriptor() :
		RN::RendererDescriptor(RNCSTR("net.uberpixel.rendering.vulkan"), RNCSTR("Vulkan"))
	{}

	Renderer *VulkanRendererDescriptor::CreateRenderer(RenderingDevice *tdevice)
	{
		VulkanDevice *device = static_cast<VulkanDevice *>(tdevice);
		if(device->CreateDevice(_instance->GetDeviceExtensions()))
		{
			vk::init_dispatch_table_bottom(_instance->GetInstance(), device->GetDevice());

			VulkanRenderer *renderer = new VulkanRenderer(this, device);
			return renderer;
		}

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
