//
//  RND3D12GPUBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "d3dx12.h"
#include "RND3D12Renderer.h"
#include "RND3D12Internals.h"
#include "RND3D12GPUBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12GPUBuffer, GPUBuffer)

	D3D12GPUBuffer::D3D12GPUBuffer(const void *data, size_t length) : _length(length), _bufferResourceUpload(nullptr)
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		renderer->_internals->device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(_length),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&_bufferResource));

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
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());
		renderer->_internals->device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(_length),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_bufferResourceUpload));

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		void* vertexDataBegin = nullptr;
		RN_ASSERT(_bufferResourceUpload, "blubb");
		_bufferResourceUpload->Map(0, &CD3DX12_RANGE(0, _length), &vertexDataBegin);
		return vertexDataBegin;
	}

	void D3D12GPUBuffer::InvalidateRange(const Range &range)
	{
		RN_ASSERT(_bufferResourceUpload, "Nothing to invalidate! Call \"GetBuffer()\" first.");

		_bufferResourceUpload->Unmap(0, nullptr);

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		renderer->_internals->WaitForGpu();
		renderer->_internals->commandAllocators[renderer->_internals->frameIndex]->Reset();
		renderer->_internals->commandList->Reset(renderer->_internals->commandAllocators[renderer->_internals->frameIndex], NULL);

		renderer->_internals->commandList->CopyBufferRegion(_bufferResource, range.origin, _bufferResourceUpload, range.origin, range.length);
		renderer->_internals->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(static_cast<ID3D12Resource*>(_bufferResource), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		//D3D12_RESOURCE_STATE_INDEX_BUFFER
		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		renderer->_internals->commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { renderer->_internals->commandList };
		renderer->_internals->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		renderer->_internals->WaitForGpu();
		_bufferResourceUpload->Release();
		_bufferResourceUpload = nullptr;
	}

	size_t D3D12GPUBuffer::GetLength() const
	{
		return _length;
	}
}
