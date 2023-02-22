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

#define kRNMinimumDynamicBufferSize 10*1000*1000 //10mb for constants to normally fit everything into a single buffer, even for complex scenes.
#define kRNDynamicBufferAlignement 256

namespace RN
{
	class Renderer;
	class GPUBuffer;
	class VulkanDynamicBufferPool;

	class VulkanDynamicBuffer : public Object
	{
	public:
		friend VulkanDynamicBufferPool;

		VulkanDynamicBuffer(Renderer *renderer, size_t size, GPUResource::UsageOptions usageOptions);
		~VulkanDynamicBuffer();

		VKAPI GPUBuffer *Advance(size_t currentFrame, size_t completedFrame);
		GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		VKAPI void Reset();
		VKAPI size_t Allocate(size_t size, bool align);
		VKAPI size_t Reserve(size_t size);
		VKAPI void Unreserve(size_t size);

	private:
		std::vector<GPUBuffer*> _buffers;
		std::vector<size_t> _bufferFrames;
		size_t _bufferIndex;

		size_t _sizeUsed;
		size_t _offsetToFreeData;
		size_t _totalSize;
		size_t _sizeReserved;

		GPUResource::UsageOptions _usageOptions;

		RNDeclareMetaAPI(VulkanDynamicBuffer, VKAPI)
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
		VulkanDynamicBuffer *dynamicBuffer;
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
