//
//  RNMetalUniformBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALUNIFORMBUFFER_H_
#define __RAYNE_METALUNIFORMBUFFER_H_

#include "RNMetal.h"
#include "RNMetalStateCoordinator.h"

#define kRNMetalUniformBufferCount 3
#define kRNMinimumUniformBufferSize 1*1000*1000
#define kRNUniformBufferAlignement 256

namespace RN
{
	class Renderer;
	class GPUBuffer;
	class MetalRenderingStateUniformBufferArgument;
	class MetalUniformBufferPool;
	
	class MetalUniformBuffer : public Object
	{
	public:
		friend MetalUniformBufferPool;
		
		MTLAPI MetalUniformBuffer(Renderer *renderer, size_t size);
		MTLAPI ~MetalUniformBuffer();
		
		MTLAPI GPUBuffer *Advance();
		MTLAPI GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }
		
		MTLAPI size_t Allocate(size_t size);
		MTLAPI void Free(size_t offset, size_t size);
		
	private:
		GPUBuffer *_buffers[kRNMetalUniformBufferCount];
		size_t _bufferIndex;
		
		size_t _sizeUsed;
		size_t _offsetToFreeData;
		size_t _totalSize;
		
		RNDeclareMetaAPI(MetalUniformBuffer, MTLAPI)
	};
	
	
	class MetalUniformBufferReference : public Object
	{
	public:
		MetalUniformBufferReference();
		~MetalUniformBufferReference();
		
		uint32 shaderResourceIndex;
		uint32 offset;
		uint32 size;
		MetalUniformBuffer *uniformBuffer;
		
	private:
		
		RNDeclareMetaAPI(MetalUniformBufferReference, MTLAPI)
	};
	

	class MetalUniformBufferPool
	{
	public:
		MetalUniformBufferPool();
		~MetalUniformBufferPool();
		MetalUniformBufferReference *GetUniformBufferReference(uint32 size, uint32 index);
		void Update(Renderer *renderer);
		void InvalidateAllBuffers();
		
	private:
		Array *_uniformBuffers;
		Array *_newReferences;
	};
}

#endif /* __RAYNE_METALUNIFORMBUFFER_H_ */
