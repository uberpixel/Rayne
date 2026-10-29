//
//  RNMetalDynamicGPUBuffer.h
//  Rayne
//
//  Copyright 2025 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALDYNAMICGPUBUFFER_H_
#define __RAYNE_METALDYNAMICGPUBUFFER_H_

#include "RNMetalGPUBuffer.h"

namespace RN
{
	class MetalDynamicGPUBuffer : public MetalGPUBuffer
	{
	public:
		friend class MetalRenderer;

		// GPUBuffer overrides
		MTLAPI void *GetBuffer() final;
		MTLAPI void UnmapBuffer() final;
		MTLAPI void InvalidateRange(const Range &range) final;
		MTLAPI void FlushRange(const Range &range, bool discard = false) final;
		MTLAPI size_t GetLength() const final { return _length; }
		MTLAPI GPUBuffer *GetActiveBuffer() const final;
		
	protected:
		MTLAPI MetalDynamicGPUBuffer(id<MTLDevice> device, size_t length, MTLResourceOptions options);
		MTLAPI ~MetalDynamicGPUBuffer() override;

	private:
		static constexpr uint32 kBufferCount = 3;
		id<MTLBuffer> _buffers[kBufferCount];
		MetalGPUBuffer *_activeBuffers[kBufferCount];
		size_t _length;
		uint32 _activeIndex;
		uint32 _writeIndex;
	};
}

#endif /* __RAYNE_METALDYNAMICGPUBUFFER_H_ */
