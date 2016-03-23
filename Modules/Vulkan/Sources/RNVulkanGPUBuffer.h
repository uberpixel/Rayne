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
		VulkanGPUBuffer(void *data, size_t length);
		~VulkanGPUBuffer() override;

		VkBuffer _buffer;
		VkDeviceMemory _memory;

		void *_data;
		size_t _length;

		RNDeclareMetaAPI(VulkanGPUBuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANGPUBUFFER_H_ */
