//
//  RNMetalGPUBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalGPUBuffer.h"

namespace RN
{
	RNDefineMeta(MetalGPUBuffer, GPUBuffer)

	MetalGPUBuffer::MetalGPUBuffer(void *data) :
		_buffer(data)
	{}

	MetalGPUBuffer::~MetalGPUBuffer()
	{
		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		[buffer release];
	}

	void *MetalGPUBuffer::GetBuffer()
	{
		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		return [buffer contents];
	}

	void MetalGPUBuffer::UnmapBuffer()
	{
		//Since metal is designed for shared memory, mapping and unmapping buffers does not appear to be a thing, but private buffers also exist and can't be mapped
	}

	void MetalGPUBuffer::InvalidateRange(const Range &range)
	{
		//This apparently happens just by the gpu writing into the buffer and finishing that operation
	}

	void MetalGPUBuffer::FlushRange(const Range &range)
	{
		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		[buffer didModifyRange:NSMakeRange(range.origin, range.length)];
	}

	size_t MetalGPUBuffer::GetLength() const
	{
		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		return [buffer length];
	}
}
