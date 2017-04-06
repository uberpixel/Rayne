//
//  RND3D12UniformBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12UNIFORMBUFFER_H_
#define __RAYNE_D3D12UNIFORMBUFFER_H_

#include "RND3D12.h"
#include "RND3D12StateCoordinator.h"

#define kRND3D12UniformBufferCount 3

namespace RN
{
	class Renderer;
	class GPUBuffer;

	class D3D12UniformBuffer : public Object
	{
	public:
		D3D12UniformBuffer(Renderer *renderer, size_t size);
		~D3D12UniformBuffer();

		GPUBuffer *Advance();
		GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		size_t GetCurrentBufferIndex() const { return _bufferIndex; }

	private:
		GPUBuffer *_buffers[kRND3D12UniformBufferCount];
		size_t _bufferIndex;

		RNDeclareMetaAPI(D3D12UniformBuffer, D3DAPI)
	};
}

#endif /* __RAYNE_D3D12UNIFORMBUFFER_H_ */
