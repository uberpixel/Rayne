//
//  RND3D12GPUBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12GPUBUFFER_H_
#define __RAYNE_D3D12GPUBUFFER_H_

#include "RND3D12.h"

namespace RN
{
	class D3D12Renderer;
	class D3D12Resource;

	class D3D12GPUBuffer : public GPUBuffer
	{
	public:
		friend class D3D12Renderer;

		D3DAPI void *GetBuffer() final;
		D3DAPI void UnmapBuffer() final;
		D3DAPI void InvalidateRange(const Range &range) final;
		D3DAPI void FlushRange(const Range &range) final;
		D3DAPI size_t GetLength() const final;

		ID3D12Resource *GetD3D12Resource() const;

	private:
		D3D12GPUBuffer(const void *data, size_t length, GPUResource::UsageOptions usageOptions);
		~D3D12GPUBuffer() override;

		D3D12Resource *_resource;

		RNDeclareMetaAPI(D3D12GPUBuffer, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12GPUBUFFER_H_ */
