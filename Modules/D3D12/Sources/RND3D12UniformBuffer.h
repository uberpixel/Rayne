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

namespace RN
{
	class Renderer;
	class GPUBuffer;

	class D3D12UniformBuffer : public Object
	{
	public:
		D3D12UniformBuffer(size_t size);
		~D3D12UniformBuffer();

		GPUBuffer *Advance(size_t currentFrame, size_t completedFrame);
		GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		size_t GetCurrentBufferIndex() const { return _bufferIndex; }

	private:
		std::vector<GPUBuffer*> _buffers;
		std::vector<size_t> _bufferFrames;
		size_t _bufferIndex;
		size_t _size;

		RNDeclareMetaAPI(D3D12UniformBuffer, D3DAPI)
	};
}

#endif /* __RAYNE_D3D12UNIFORMBUFFER_H_ */
