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

namespace RN
{
	class Renderer;
	class GPUBuffer;
	class MetalRenderingStateUniformBufferArgument;

	class MetalUniformBuffer : public Object
	{
	public:
		MTLAPI MetalUniformBuffer(Renderer *renderer, size_t size, uint32 index);
		MTLAPI ~MetalUniformBuffer();

		MTLAPI GPUBuffer *Advance();
		MTLAPI GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		size_t GetIndex() const { return _index; }

	private:
		size_t _index;
		GPUBuffer *_buffers[kRNMetalUniformBufferCount];
		size_t _bufferIndex;

		RNDeclareMetaAPI(MetalUniformBuffer, MTLAPI)
	};
}

#endif /* __RAYNE_METALUNIFORMBUFFER_H_ */
