//
//  RND3D12GPUBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12GPUBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12GPUBuffer, GPUBuffer)

	D3D12GPUBuffer::D3D12GPUBuffer(void *data) :
		_buffer(data)
	{}

	D3D12GPUBuffer::~D3D12GPUBuffer()
	{
/*		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		[buffer release];*/
	}

	void *D3D12GPUBuffer::GetBuffer() const
	{
/*		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		return [buffer contents];*/
		return nullptr;
	}

	void D3D12GPUBuffer::InvalidateRange(const Range &range)
	{
/*		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		[buffer didModifyRange:NSMakeRange(range.origin, range.length)];*/
	}

	size_t D3D12GPUBuffer::GetLength() const
	{
/*		id<MTLBuffer> buffer = (id<MTLBuffer>)_buffer;
		return [buffer length];*/

		return 0;
	}
}
