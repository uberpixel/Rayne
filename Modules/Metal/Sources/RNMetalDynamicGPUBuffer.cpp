//
//  RNMetalDynamicGPUBuffer.cpp
//  Rayne
//
//  Copyright 2025 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalDynamicGPUBuffer.h"

namespace RN
{
	MetalDynamicGPUBuffer::MetalDynamicGPUBuffer(id<MTLDevice> device, size_t length, MTLResourceOptions options) : MetalGPUBuffer(nullptr), _length(length), _activeIndex(0), _writeIndex(1)
	{
		for(uint32 i = 0; i < kBufferCount; ++i)
		{
			_buffers[i] = [device newBufferWithLength:length options:options];
			_activeBuffers[i] = new MetalGPUBuffer([_buffers[i] retain]);
		}

		_buffer = _buffers[_activeIndex];
	}

	MetalDynamicGPUBuffer::~MetalDynamicGPUBuffer()
	{
		for(uint32 i = 0; i < kBufferCount; ++i)
		{
			SafeRelease(_activeBuffers[i]);
			[_buffers[i] release];
		}
		_buffer = nullptr;
	}

	void *MetalDynamicGPUBuffer::GetBuffer()
	{
		id<MTLBuffer> buffer = _buffers[_writeIndex];
		return [buffer contents];
	}

	void MetalDynamicGPUBuffer::UnmapBuffer()
	{
		// No-op on Metal shared buffers
	}

	void MetalDynamicGPUBuffer::InvalidateRange(const Range &range)
	{
		// No-op for CPU writes
	}

	void MetalDynamicGPUBuffer::FlushRange(const Range &range, bool)
	{
	#if RN_PLATFORM_MAC_OS
		id<MTLBuffer> writeBuffer = _buffers[_writeIndex];
		[writeBuffer didModifyRange:NSMakeRange(range.origin, range.length)];
	#endif
		// Publish the written buffer for binding in this frame
		_activeIndex = _writeIndex;
		_buffer = _buffers[_activeIndex];

		// Advance write index for the next frame
		_writeIndex = (_writeIndex + 1) % kBufferCount;
	}

	GPUBuffer *MetalDynamicGPUBuffer::GetActiveBuffer() const
	{
		return _activeBuffers[_activeIndex];
	}
}
