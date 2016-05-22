//
//  RND3D12GPUBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "d3dx12.h"
#include "RND3D12Renderer.h"
#include "RND3D12GPUBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12GPUBuffer, GPUBuffer)

	D3D12GPUBuffer::D3D12GPUBuffer(const void *data, size_t length) : 
		_length(length),
		_bufferResourceUpload(nullptr)
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());


		if(data)
		{
			void* vertexDataBegin = GetBuffer();
			memcpy(vertexDataBegin, data, length);
			Invalidate();
		}
	}

	D3D12GPUBuffer::~D3D12GPUBuffer()
	{
		_bufferResource->Release();
	}

	void *D3D12GPUBuffer::GetBuffer()
	{
		return nullptr;
	}

	void D3D12GPUBuffer::InvalidateRange(const Range &range)
	{
		
	}

	size_t D3D12GPUBuffer::GetLength() const
	{
		return _length;
	}
}
