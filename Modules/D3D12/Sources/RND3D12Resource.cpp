//
//  RND3D12Resource.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "d3dx12.h"
#include "RND3D12Renderer.h"
#include "RND3D12Resource.h"
#include "RND3D12Internals.h"

namespace RN
{
	RNDefineMeta(D3D12Resource, Object)

		D3D12Resource::D3D12Resource(D3D12Device *device, size_t length, ResourceType resourceType) :
		_device(device),
		_resourceType(resourceType),
		_length(length),
		_transferPointer(nullptr),
		_isRecording(false),
		_allocation(nullptr),
		_transferAllocation(nullptr)
	{
		_resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(_length);

		HRESULT hr = 0;
		if(_resourceType == ResourceType::Uniform)
		{
			_resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			hr = device->GetMemoryAllocator()->CreateResource(&allocationDesc, &_resourceDescription, _resourceState, NULL, &_allocation, IID_NULL, NULL);
		}
		else
		{
			if(resourceType == ResourceType::Vertex)
				_resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			else if(resourceType == ResourceType::Index)
				_resourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			else
				_resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			hr = device->GetMemoryAllocator()->CreateResource(&allocationDesc, &_resourceDescription, _resourceState, NULL, &_allocation, IID_NULL, NULL);
		}

		if(FAILED(hr))
		{
			_allocation = nullptr;
			_length = 0;
			return;
		}
	}

	D3D12Resource::~D3D12Resource()
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		if(_allocation)
			renderer->AddFrameResouce(_allocation);

		if(_transferAllocation)
			renderer->AddFrameResouce(_transferAllocation);
	}

	void *D3D12Resource::GetUploadBuffer()
	{
		if(!_allocation)
			return nullptr;

		if(_resourceType == ResourceType::Uniform)
		{
			if(!_transferPointer)
			{
				CD3DX12_RANGE readRange(0, 0);
				_allocation->GetResource()->Map(0, &readRange, &_transferPointer);
			}
			
			return _transferPointer;
		}
		else
		{
			if(_isRecording)
				return _transferPointer;

			_isRecording = true;

			_transferAllocation = GetTransferAllocation();

			CD3DX12_RANGE readRange(0, 0);
			_transferAllocation->GetResource()->Map(0, &readRange, &_transferPointer);

			return _transferPointer;
		}
	}

	void D3D12Resource::Flush()
	{
		if(_resourceType == ResourceType::Uniform)
			return;

		if (!_transferAllocation) return;

		_transferAllocation->GetResource()->Unmap(0, nullptr);
		_isRecording = false;
		_transferPointer = nullptr;

		D3D12Renderer *renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		D3D12CommandList *commandList = renderer->GetCommandList();
		D3D12MA::Allocation *transferAllocation = _transferAllocation;
		_transferAllocation = nullptr;
		commandList->SetFinishedCallback([transferAllocation] {
			transferAllocation->Release();
		});

		D3D12_RESOURCE_STATES originalResourceState = _resourceState;
		if(!(_resourceState & D3D12_RESOURCE_STATE_COPY_DEST))
		{
			SetResourceState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
		}

		commandList->GetCommandList()->CopyResource(_allocation->GetResource(), transferAllocation->GetResource());

		if(!(_resourceState & originalResourceState))
		{
			SetResourceState(commandList, originalResourceState);
		}

		commandList->End();
		renderer->SubmitCommandList(commandList);
	}

	ID3D12Resource *D3D12Resource::GetD3D12Resource() const
	{
		return _allocation->GetResource();
	}

	D3D12MA::Allocation *D3D12Resource::GetTransferAllocation()
	{
		D3D12MA::Allocation *transferAllocation = nullptr;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		HRESULT hr = _device->GetMemoryAllocator()->CreateResource(&allocationDesc, &_resourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &transferAllocation, IID_NULL, NULL);

		if(FAILED(hr))
		{
			transferAllocation = nullptr;
		}

		return transferAllocation;
	}

	size_t D3D12Resource::GetLength()
	{
		return _length;
	}

	void D3D12Resource::SetResourceState(D3D12CommandList *commandList, D3D12_RESOURCE_STATES resourceState)
	{
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_allocation->GetResource(), _resourceState, resourceState));
		_resourceState = resourceState;
	}
}
