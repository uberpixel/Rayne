//
//  RND3D12RendererDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12RendererDescriptor.h"
#include "RND3D12Renderer.h"
#include "RND3D12Device.h"

namespace RN
{
	RNDefineMeta(D3D12RendererDescriptor, RendererDescriptor)

	void D3D12RendererDescriptor::InitialWakeUp(MetaClass *meta)
	{
		if(meta == D3D12RendererDescriptor::GetMetaClass())
		{
			D3D12RendererDescriptor *descriptor = new D3D12RendererDescriptor();
			GetExtensionPoint()->AddExtension(descriptor, -5);
			descriptor->Release();
		}
	}

	D3D12RendererDescriptor::D3D12RendererDescriptor() :
		RN::RendererDescriptor(RNCSTR("net.uberpixel.rendering.d3d12"), RNCSTR("D3D12")),
		_devices(nullptr)
	{}


	Renderer *D3D12RendererDescriptor::CreateRenderer(RenderingDevice *device)
	{
		return nullptr;
	}
	bool D3D12RendererDescriptor::CanCreateRenderer() const
	{
		return (_devices && _devices->GetCount() > 0);
	}

	void D3D12RendererDescriptor::PrepareWithSettings(const Dictionary *settings)
	{
		if(::CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), reinterpret_cast<void **>(&_factory)) != S_OK)
			return;

		_devices = new Array();

		UINT i = 0;
		while(1)
		{
			IDXGIAdapter1 *adapter;
			HRESULT result = _factory->EnumAdapters1(i, &adapter);

			if(result != S_OK)
				break;

			D3D12Device *device = new D3D12Device(adapter);
			_devices->AddObject(device);
			device->Release();

			RNDebug("Found adapter " << device);

			i++;
		}
	}

	const Array *D3D12RendererDescriptor::GetDevices() const
	{
		return _devices;
	}
}
