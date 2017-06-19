//
//  RND3D12Internals.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Internals.h"
#include "RND3D12Renderer.h"

namespace RN
{
	RNDefineMeta(D3D12DescriptorHeap, Object)

	D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12Device *device) : _device(device), _heap(nullptr), _size(0), _fenceValue(0)
	{
		//TODO: maybe generalize for the different heap types... currently only srv/cbv/uav heap
		_handleIncrement = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3D12DescriptorHeap::~D3D12DescriptorHeap()
	{
		_heap->Release();
	}

	void D3D12DescriptorHeap::Reset(size_t size)
	{
		if(_size < size)
		{
			RN_ASSERT(size <= 1000000, "Descriptor heap is too big and may not be supported on some hardware!");

			if(_heap)
			{
				Renderer::GetActiveRenderer()->Downcast<D3D12Renderer>()->AddFrameResouce(_heap, _fenceValue);
			}

			D3D12_DESCRIPTOR_HEAP_DESC srvCbvHeapDesc = {};
			srvCbvHeapDesc.NumDescriptors = size;
			srvCbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvCbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			_device->CreateDescriptorHeap(&srvCbvHeapDesc, IID_PPV_ARGS(&_heap));

			_handleCPU = _heap->GetCPUDescriptorHandleForHeapStart();
			_handleGPU = _heap->GetGPUDescriptorHandleForHeapStart();

			_size = size;
		}
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUHandle(UINT index) const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(_handleCPU, index, _handleIncrement);
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHandle(UINT index) const
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(_handleGPU, index, _handleIncrement);
	}


	RNDefineMeta(D3D12CommandList, Object)

	D3D12CommandList::D3D12CommandList(ID3D12Device *device) : _device(device), _isOpen(true), _finishedCallback(nullptr)
	{
		_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator));
		_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator, nullptr, IID_PPV_ARGS(&_commandList));
	}

	D3D12CommandList::~D3D12CommandList()
	{
		_commandList->Release();
		_commandAllocator->Release();
	}

	void D3D12CommandList::Begin()
	{
		if(!_isOpen)
		{
			_commandAllocator->Reset();
			_commandList->Reset(_commandAllocator, nullptr);
		}
	}

	void D3D12CommandList::End()
	{
		_commandList->Close();
		_isOpen = false;
	}

	void D3D12CommandList::SetFinishedCallback(std::function<void()> callback)
	{
		_finishedCallback = callback;
	}

	void D3D12CommandList::Finish()
	{
		if(_finishedCallback)
		{
			_finishedCallback();
			_finishedCallback = nullptr;
		}
	}
}
