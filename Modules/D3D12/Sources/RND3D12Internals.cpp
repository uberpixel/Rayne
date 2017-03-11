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
	RNDefineMeta(D3D12CommandList, Object)
	RNDefineMeta(D3D12CommandListWithCallback, D3D12CommandList)

	D3D12CommandList::D3D12CommandList(ID3D12Device *device) : _device(device), _isOpen(true)
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

	D3D12CommandListWithCallback::D3D12CommandListWithCallback(ID3D12Device *device) : D3D12CommandList(device)
	{

	}

	D3D12CommandListWithCallback::~D3D12CommandListWithCallback()
	{
		_finishedCallback();
	}

	void D3D12CommandListWithCallback::SetFinishedCallback(std::function<void()> callback)
	{
		_finishedCallback = callback;
	}
}
