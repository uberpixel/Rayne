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

	D3D12GPUBuffer::D3D12GPUBuffer(const void *data, size_t length) : _length(length)
	{
/*		WaitForGpu();

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(_commandAllocators[_frameIndex]->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(_commandList->Reset(_commandAllocators[_frameIndex].Get(), NULL));*/

		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());
		ID3D12Resource *bufferResourceUpload;

		renderer->_internals->device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(_length),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&_bufferResource));

		renderer->_internals->device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(_length),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&bufferResourceUpload));

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		UINT8* pVertexDataBegin;
		bufferResourceUpload->Map(0, &CD3DX12_RANGE(0, _length), reinterpret_cast<void**>(&pVertexDataBegin));
		memcpy(pVertexDataBegin, data, _length);
		bufferResourceUpload->Unmap(0, nullptr);

		renderer->_internals->commandList->CopyBufferRegion(_bufferResource, 0, bufferResourceUpload, 0, _length);
		renderer->_internals->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(static_cast<ID3D12Resource*>(_bufferResource), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		renderer->_internals->commandList->Close();
		ID3D12CommandList* ppCommandLists[] = {renderer->_internals->commandList};
		renderer->_internals->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

/*		WaitForGpu();

		return vertexBuffer;*/
	}

	D3D12GPUBuffer::~D3D12GPUBuffer()
	{
		_bufferResource->Release();
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

		return _length;
	}
}
