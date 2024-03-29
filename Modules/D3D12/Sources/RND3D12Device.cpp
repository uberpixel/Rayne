//
//  RND3D12Device.cpp
//  Rayne-D3D12
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Device.h"

namespace RN
{
	RNDefineMeta(D3D12Device, RenderingDevice)

	static String *NameForAdapter(IDXGIAdapter1 *adapter)
	{
		DXGI_ADAPTER_DESC1 descriptor = { 0 };
		adapter->GetDesc1(&descriptor);

		return RNSTR(descriptor.Description);
	}

	static RenderingDevice::Descriptor DescriptorForAdapter(IDXGIAdapter1 *adapter)
	{
		DXGI_ADAPTER_DESC1 dxgiDescriptor = { 0 };
		adapter->GetDesc1(&dxgiDescriptor);

		RenderingDevice::Descriptor descriptor = { 0 };

		descriptor.vendorID = dxgiDescriptor.VendorId;
		descriptor.driverVersion = RNVersionMake(dxgiDescriptor.Revision, 0, 0);
		descriptor.apiVersion = RNVersionMake(12, 0, 0);

		return descriptor;
	}

	D3D12Device::D3D12Device(IDXGIAdapter1 *adapter) :
		RenderingDevice(NameForAdapter(adapter), DescriptorForAdapter(adapter)),
		_adapter(adapter),
		_device(nullptr)
	{}

	D3D12Device::~D3D12Device()
	{
		_memoryAllocator->Release();
		_device->Release();
	}

	bool D3D12Device::CreateDevice()
	{
		//TODO: Handle different feature levels
		HRESULT result = ::D3D12CreateDevice(_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));

		if(result != S_OK) return false;

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = _device;
		allocatorDesc.pAdapter = _adapter;
		result = D3D12MA::CreateAllocator(&allocatorDesc, &_memoryAllocator);

		return (result == S_OK);
	}
}
