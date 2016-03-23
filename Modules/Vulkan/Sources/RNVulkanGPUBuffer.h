//
//  RNVulkanGPUBuffer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANGPUBUFFER_H_
#define __RAYNE_VULKANGPUBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	class VulkanGPUBuffer : public GPUBuffer
	{
	public:
		friend class VulkanRenderer;

		VKAPI void *GetBuffer() final;
		VKAPI void InvalidateRange(const Range &range) final;
		VKAPI size_t GetLength() const final;

	private:
		VulkanGPUBuffer(VulkanRenderer *renderer, void *data, size_t length, GPUResource::UsageOptions usageOption);
		~VulkanGPUBuffer() override;

		VulkanRenderer *_renderer;

		VkBuffer _buffer;
		VkDeviceMemory _memory;

		size_t _length;
		void *_mappedBuffer;

		RNDeclareMetaAPI(VulkanGPUBuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANGPUBUFFER_H_ */
