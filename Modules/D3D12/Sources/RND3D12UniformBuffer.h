//
//  RND3D12UniformBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12UNIFORMBUFFER_H_
#define __RAYNE_D3D12UNIFORMBUFFER_H_

#include "RND3D12.h"
#include "RND3D12StateCoordinator.h"

#define kRNMinimumUniformBufferSize 1*1000*1000
#define kRNUniformBufferAlignement 256

namespace RN
{
	class Renderer;
	class GPUBuffer;
	class D3D12UniformBufferPool;

	class D3D12UniformBuffer : public Object
	{
	public:
		friend D3D12UniformBufferPool;

		D3D12UniformBuffer(size_t size);
		~D3D12UniformBuffer();

		D3DAPI GPUBuffer *Advance(size_t currentFrame, size_t completedFrame);
		GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		D3DAPI size_t Allocate(size_t size);
		D3DAPI void Free(size_t offset, size_t size);

	private:
		std::vector<GPUBuffer*> _buffers;
		std::vector<size_t> _bufferFrames;
		size_t _bufferIndex;
		
		size_t _sizeUsed;
		size_t _offsetToFreeData;
		size_t _totalSize;

		RNDeclareMetaAPI(D3D12UniformBuffer, D3DAPI)
	};

	class D3D12UniformBufferReference : public Object
	{
	public:
		D3D12UniformBufferReference();
		~D3D12UniformBufferReference();

		uint32 shaderResourceIndex;
		uint32 offset;
		uint32 size;
		D3D12UniformBuffer *uniformBuffer;

	private:

		RNDeclareMetaAPI(D3D12UniformBufferReference, D3DAPI)
	};


	class D3D12UniformBufferPool
	{
	public:
		D3D12UniformBufferPool();
		~D3D12UniformBufferPool();
		D3D12UniformBufferReference *GetUniformBufferReference(uint32 size, uint32 index);
		void Update(size_t currentFrame, size_t completedFrame);
		void InvalidateAllBuffers();

	private:
		Array *_uniformBuffers;
		Array *_newReferences;
	};
}

#endif /* __RAYNE_D3D12UNIFORMBUFFER_H_ */
