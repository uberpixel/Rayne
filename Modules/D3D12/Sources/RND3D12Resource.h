//
//  RND3D12Resource.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12RESOURCE_H_
#define __RAYNE_D3D12RESOURCE_H_

#include "RND3D12.h"
#include <D3D12MemAlloc.h>

namespace RN
{
	class D3D12Renderer;
	class D3D12Resource : public Object
	{
	public:
		friend class D3D12Renderer;

		enum ResourceType
		{
			Uniform,
			Vertex,
			Index,
			Texture
		};

		D3D12Resource(D3D12Device *device, size_t length, ResourceType resourceType);
		~D3D12Resource() override;

		void *GetUploadBuffer();
		void Flush();

		ID3D12Resource *GetD3D12Resource() const;

		size_t GetLength();

		void SetResourceState(D3D12CommandList *commandList, D3D12_RESOURCE_STATES resourceState);

	private:
		D3D12MA::Allocation* GetTransferAllocation();

		D3D12Device *_device;

		ResourceType _resourceType;
		size_t _length;

		D3D12MA::Allocation *_allocation;
		D3D12MA::Allocation *_transferAllocation;

		D3D12_RESOURCE_STATES _resourceState;
		bool _isRecording;
		void *_transferPointer;

		CD3DX12_RESOURCE_DESC _resourceDescription;

		RNDeclareMetaAPI(D3D12Resource, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12RESOURCE_H_ */
