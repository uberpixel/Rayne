//
//  RNVulkanStaticGPUBuffer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANSTATICGPUBUFFER_H_
#define __RAYNE_VULKANSTATICGPUBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanRenderer.h"
#include "RNVulkanGPUBuffer.h"

#include <vk_mem_alloc.h>

namespace RN
{
	class VulkanStaticGPUBuffer : public VulkanGPUBuffer
	{
	public:
		friend class VulkanRenderer;

		VKAPI void *GetBuffer() final;
		VKAPI void UnmapBuffer() final;
		VKAPI void InvalidateRange(const Range &range) final;
		VKAPI void FlushRange(const Range &range) final;
		VKAPI size_t GetLength() const final;

		VkBuffer GetVulkanBuffer() const final;

	private:
		VulkanStaticGPUBuffer(VulkanRenderer *renderer, void *data, size_t length, GPUResource::UsageOptions usageOption, GPUResource::AccessOptions accessOptions);
		~VulkanStaticGPUBuffer() override;

		VulkanRenderer *_renderer;

		VkBuffer _buffer;
		VmaAllocation _allocation;

		VkBuffer _stagingBuffer;
		VmaAllocation _stagingAllocation;

		bool _isHostVisible;
		size_t _length;
		std::atomic<void *> _mappedBuffer;

		RNDeclareMetaAPI(VulkanStaticGPUBuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSTATICGPUBUFFER_H_ */
