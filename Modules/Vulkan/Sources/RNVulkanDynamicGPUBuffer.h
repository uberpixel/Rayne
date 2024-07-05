//
//  RNVulkanDynamicBuffer.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANDYNAMICBUFFER_H_
#define __RAYNE_VULKANDYNAMICBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanStateCoordinator.h"
#include "RNVulkanGPUBuffer.h"
#include "RNVulkanStaticGPUBuffer.h"

#define kRNMinimumDynamicBufferSize 10*1000*1000 //10mb for constants to normally fit everything into a single buffer, even for complex scenes.
#define kRNDynamicBufferAlignement 256

namespace RN
{
	class Renderer;
	class GPUBuffer;
	class VulkanDynamicBufferPool;

	class VulkanDynamicGPUBuffer : public VulkanGPUBuffer
	{
	public:
		friend VulkanDynamicBufferPool;

		VulkanDynamicGPUBuffer(Renderer *renderer, size_t size, GPUResource::UsageOptions usageOptions);
		~VulkanDynamicGPUBuffer();

		VKAPI GPUBuffer *Advance(size_t currentFrame, size_t completedFrame);
		GPUBuffer *GetActiveGPUBuffer() const { return _buffers[_bufferIndex]; }
		void *GetBuffer() override { return _buffers[_bufferIndex]->GetBuffer(); }

		void UnmapBuffer() override { } //Can't be unmapped and is just permanently mapped
		void InvalidateRange(const Range &range) override { _buffers[_bufferIndex]->InvalidateRange(range); }
		VKAPI void FlushRange(const Range &range) override;
		RNAPI virtual size_t GetLength() const override { return _totalSize; }

		virtual VkBuffer GetVulkanBuffer() const override { return _buffers[_bufferIndex]->GetVulkanBuffer(); }

		//These are used for uniform buffers internally as they suballocate smaller pieces of big dynamic buffers
		VKAPI void Reset();
		VKAPI size_t Allocate(size_t size, bool align);
		VKAPI size_t Reserve(size_t size);
		VKAPI void Unreserve(size_t size);

	private:
		std::vector<VulkanStaticGPUBuffer*> _buffers;
		std::vector<size_t> _bufferFrames;
		size_t _bufferIndex;

		size_t _sizeUsed;
		size_t _offsetToFreeData;
		size_t _totalSize;
		size_t _sizeReserved;

		GPUResource::UsageOptions _usageOptions;

		RNDeclareMetaAPI(VulkanDynamicGPUBuffer, VKAPI)
	};

	class VulkanDynamicBufferReference : public Object
	{
	public:
		VulkanDynamicBufferReference();
		~VulkanDynamicBufferReference();

		uint32 shaderResourceIndex;
		uint32 offset;
		uint32 size;
		uint32 reservedSize;
		VulkanDynamicGPUBuffer *dynamicBuffer;
		GPUResource::UsageOptions usageOptions;

	private:

	RNDeclareMetaAPI(VulkanDynamicBufferReference, VKAPI)
	};


	class VulkanDynamicBufferPool
	{
	public:
		VulkanDynamicBufferPool();
		~VulkanDynamicBufferPool();
		VulkanDynamicBufferReference *GetDynamicBufferReference(uint32 size, uint32 index, GPUResource::UsageOptions usageOptions);
		void UpdateDynamicBufferReference(VulkanDynamicBufferReference *reference, bool align);
		void Update(Renderer *renderer, size_t currentFrame, size_t completedFrame);
		void FlushAllBuffers();

	private:
		Array *_dynamicBuffers;
		Array *_newReferences;
	};
}

#endif /* __RAYNE_VULKANDYNAMICBUFFER_H_ */
