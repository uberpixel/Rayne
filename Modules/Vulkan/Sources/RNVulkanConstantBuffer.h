//
//  RNVulkanConstantBuffer.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANCONSTANTBUFFER_H_
#define __RAYNE_VULKANCONSTANTBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanStateCoordinator.h"

#define kRNMinimumConstantBufferSize 1*1000*1000
#define kRNConstantBufferAlignement 256

namespace RN
{
	class Renderer;
	class GPUBuffer;
	class VulkanConstantBufferPool;

	class VulkanConstantBuffer : public Object
	{
	public:
		friend VulkanConstantBufferPool;

		VulkanConstantBuffer(Renderer *renderer, size_t size);
		~VulkanConstantBuffer();

		VKAPI GPUBuffer *Advance(size_t currentFrame, size_t completedFrame);
		VKAPI GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		VKAPI size_t Allocate(size_t size);
		VKAPI void Free(size_t offset, size_t size);

	private:
		std::vector<GPUBuffer*> _buffers;
		std::vector<size_t> _bufferFrames;
		size_t _bufferIndex;

		size_t _sizeUsed;
		size_t _offsetToFreeData;
		size_t _totalSize;

		RNDeclareMetaAPI(VulkanConstantBuffer, VKAPI)
	};

	class VulkanConstantBufferReference : public Object
	{
	public:
		VulkanConstantBufferReference();
		~VulkanConstantBufferReference();

		uint32 shaderResourceIndex;
		uint32 offset;
		uint32 size;
		VulkanConstantBuffer *constantBuffer;

	private:

	RNDeclareMetaAPI(VulkanConstantBufferReference, VKAPI)
	};


	class VulkanConstantBufferPool
	{
	public:
		VulkanConstantBufferPool();
		~VulkanConstantBufferPool();
		VulkanConstantBufferReference *GetConstantBufferReference(uint32 size, uint32 index);
		void Update(Renderer *renderer, size_t currentFrame, size_t completedFrame);
		void InvalidateAllBuffers();

	private:
		Array *_constantBuffers;
		Array *_newReferences;
	};
}

#endif /* __RAYNE_VULKANCONSTANTBUFFER_H_ */
