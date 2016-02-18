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

	static HMODULE _vulkanModule = nullptr;

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
		if(!_vulkanModule)
		{
			_vulkanModule = ::LoadLibrary("vulkan-1.dll");

			PFN_vkGetInstanceProcAddr procAddr = nullptr;

			if(_vulkanModule)
				procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(::GetProcAddress(_vulkanModule, "vkGetInstanceProcAddr"));

			if(!_vulkanModule || !procAddr)
			{
				RNError("Couldn't load Vulkan library");

				if(_vulkanModule)
				{
					::FreeLibrary(_vulkanModule);
					_vulkanModule = nullptr;
				}

				return nullptr;
			}
		}

		return false;
	}
}
