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
#include "RND3D12Resource.h"

namespace RN
{
	RNDefineMeta(D3D12GPUBuffer, GPUBuffer)

	D3D12GPUBuffer::D3D12GPUBuffer(const void *data, size_t length, UsageOptions usageOptions) :
		_resource(nullptr)
	{
		D3D12Renderer *renderer = static_cast<D3D12Renderer *>(Renderer::GetActiveRenderer());
		ID3D12Device *device = renderer->GetD3D12Device()->GetDevice();

		if(usageOptions == UsageOptions::Uniform)
		{
			//Uniform buffers need 256bit alignement
			length = (length + 255) & ~255;
		}

		D3D12Resource::ResourceType resourceType;
		switch(usageOptions)
		{
			case UsageOptions::Index:
				resourceType = D3D12Resource::ResourceType::Index;
				break;
			case UsageOptions::Vertex:
				resourceType = D3D12Resource::ResourceType::Vertex;
				break;
			case UsageOptions::Uniform:
				resourceType = D3D12Resource::ResourceType::Uniform;
				break;
		}

		_resource = new D3D12Resource(device, length, resourceType);

		if(data)
		{
			void *copyDst = GetBuffer();
			std::memcpy(copyDst, data, length);
			Invalidate();
		}
	}

	D3D12GPUBuffer::~D3D12GPUBuffer()
	{
		_resource->Release();
	}

	void *D3D12GPUBuffer::GetBuffer()
	{
		if(!_resource)
			return nullptr;

		return _resource->GetUploadBuffer();
	}

	void D3D12GPUBuffer::InvalidateRange(const Range &range)
	{
		_resource->Invalidate();
	}

	size_t D3D12GPUBuffer::GetLength() const
	{
		return _resource->GetLength();
	}

	ID3D12Resource *D3D12GPUBuffer::GetD3D12Resource() const
	{
		return _resource->GetD3D12Resource();
	}
}
