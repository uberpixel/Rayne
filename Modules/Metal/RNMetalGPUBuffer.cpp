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

	void *MetalGPUBuffer::GetBuffer() const
	{
		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		return [buffer contents];
	}

	void MetalGPUBuffer::InvalidateRange(const Range &range)
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
