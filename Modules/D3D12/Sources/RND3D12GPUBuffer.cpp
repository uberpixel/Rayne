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
		_bufferResource(nullptr)
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());
		ID3D12Device *device = renderer->GetD3D12Device()->GetDevice();

		HRESULT hr = device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(length), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_bufferResource));

		if(FAILED(hr))
		{
			_bufferResource = nullptr;
			_length = 0;
			return;
		}

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
		if(!_bufferResource)
			return nullptr;

		void *data;
		CD3DX12_RANGE readRange(0, 0);
		_bufferResource->Map(0, &readRange, &data);

		return data;
	}

	void D3D12GPUBuffer::InvalidateRange(const Range &range)
	{
		_bufferResource->Unmap(0, nullptr);
	}

	size_t D3D12GPUBuffer::GetLength() const
	{
		return _length;
	}

	ID3D12Resource *D3D12GPUBuffer::GetD3D12Buffer() const
	{
		return _bufferResource;
	}
}
