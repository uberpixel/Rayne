//
//  RNVulkanGPUBuffer.h
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANGPUBUFFER_H_
#define __RAYNE_VULKANGPUBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanRenderer.h"

#include <vk_mem_alloc.h>

namespace RN
{
	class VulkanGPUBuffer : public GPUBuffer
	{
	public:
		VKAPI void *GetBuffer() override = 0;
		VKAPI void UnmapBuffer() override = 0;
		VKAPI void InvalidateRange(const Range &range) override = 0;
		VKAPI void FlushRange(const Range &range) override = 0;
		VKAPI size_t GetLength() const override = 0;

		virtual VkBuffer GetVulkanBuffer() const = 0;

		RNDeclareMetaAPI(VulkanGPUBuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANGPUBUFFER_H_ */
