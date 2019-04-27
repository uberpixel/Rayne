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

		D3D12Resource::D3D12Resource(ID3D12Device *device, size_t length, ResourceType resourceType) :
		_device(device),
		_resourceType(resourceType),
		_length(length),
		_transferResource(nullptr),
		_transferPointer(nullptr),
		_resource(nullptr),
		_isTransfering(false),
		_isRecording(false)
	{
		_resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(_length);

		HRESULT hr = 0;
		if(_resourceType == ResourceType::Uniform)
		{
			_resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			hr = _device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &_resourceDescription, _resourceState, nullptr, IID_PPV_ARGS(&_resource));
		}
		else
		{
			if(resourceType == ResourceType::Vertex)
				_resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			else if(resourceType == ResourceType::Index)
				_resourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			else
				_resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

			hr = _device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &_resourceDescription, _resourceState, nullptr, IID_PPV_ARGS(&_resource));
		}

		if(FAILED(hr))
		{
			_resource = nullptr;
			_length = 0;
			return;
		}
	}

	D3D12Resource::~D3D12Resource()
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());

		if(_resource)
			renderer->AddFrameResouce(_resource);

		if(_transferResource)
			renderer->AddFrameResouce(_transferResource);
	}

	void *D3D12Resource::GetUploadBuffer()
	{
		if(!_resource)
			return nullptr;

		if(_resourceType == ResourceType::Uniform)
		{
			if(!_transferPointer)
			{
				CD3DX12_RANGE readRange(0, 0);
				_resource->Map(0, &readRange, &_transferPointer);
			}
			
			return _transferPointer;
		}
		else
		{
			if(_isRecording)
				return _transferPointer;

			_isRecording = true;
			_isTransfering = true;

			_transferResource = GetTransferResource();

			CD3DX12_RANGE readRange(0, 0);
			_transferResource->Map(0, &readRange, &_transferPointer);

			return _transferPointer;
		}
	}

	void D3D12Resource::Invalidate()
	{
		if(_resourceType == ResourceType::Uniform)
			return;

		_transferResource->Unmap(0, nullptr);
		_isRecording = false;
		_transferPointer = nullptr;

		D3D12Renderer *renderer = Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>();
		D3D12CommandList *commandList = renderer->GetCommandList();
		ID3D12Resource *transferResource = _transferResource;
		_transferResource = nullptr;
		commandList->SetFinishedCallback([this, transferResource] {
			_isTransfering = false;
			transferResource->Release();
		});

		D3D12_RESOURCE_STATES originalResourceState = _resourceState;
		if(!(_resourceState & D3D12_RESOURCE_STATE_COPY_DEST))
		{
			SetResourceState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
		}

		commandList->GetCommandList()->CopyResource(_resource, transferResource);

		if(!(_resourceState & originalResourceState))
		{
			SetResourceState(commandList, originalResourceState);
		}

		commandList->End();
		renderer->SubmitCommandList(commandList);
	}

	ID3D12Resource *D3D12Resource::GetD3D12Resource() const
	{
		return _resource;
	}

	ID3D12Resource *D3D12Resource::GetTransferResource()
	{
		ID3D12Resource *transferResource = nullptr;
		HRESULT hr = _device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &_resourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&transferResource));

		if(FAILED(hr))
		{
			transferResource = nullptr;
		}

		return transferResource;
	}

	size_t D3D12Resource::GetLength()
	{
		return _length;
	}

	void D3D12Resource::SetResourceState(D3D12CommandList *commandList, D3D12_RESOURCE_STATES resourceState)
	{
		commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_resource, _resourceState, resourceState));
		_resourceState = resourceState;
	}
}
