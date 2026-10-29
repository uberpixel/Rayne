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
		friend class VulkanDynamicGPUBuffer;

		VKAPI void *GetBuffer() final;
		VKAPI void UnmapBuffer() final;
		VKAPI void InvalidateRange(const Range &range) final;
		VKAPI void FlushRange(const Range &range, bool discard = false) final;
		VKAPI size_t GetLength() const final;

		VkBuffer GetVulkanBuffer() const final;
		StrongRef<GPUBuffer> GetSnapshotReference() final { return _isStreamedBacking ? this : nullptr; }

		void ReserveForSnapshot() noexcept final
		{
			_snapshotReservationCount.fetch_add(1, std::memory_order_relaxed);
		}

		void ReleaseSnapshotReservation() noexcept final
		{
			_snapshotReservationCount.fetch_sub(1, std::memory_order_release);
		}

	private:
		bool HasSnapshotReservations() const { return _snapshotReservationCount.load(std::memory_order_acquire) != 0; }
		VulkanStaticGPUBuffer(VulkanRenderer *renderer, void *data, size_t length, GPUResource::UsageOptions usageOption, GPUResource::AccessOptions accessOptions, bool isStreamedBacking = false);
		~VulkanStaticGPUBuffer() override;

		VulkanRenderer *_renderer;

		VkBuffer _buffer;
		VmaAllocation _allocation;

		VkBuffer _stagingBuffer;
		VmaAllocation _stagingAllocation;

		bool _isHostVisible;
		bool _isStreamedBacking;
		std::atomic<uint32> _snapshotReservationCount {0};
		size_t _length;
		std::atomic<void *> _mappedBuffer;

		RNDeclareMetaAPI(VulkanStaticGPUBuffer, VKAPI)
	};
}


#endif /* __RAYNE_VULKANSTATICGPUBUFFER_H_ */
