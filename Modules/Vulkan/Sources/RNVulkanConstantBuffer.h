//
//  RNVulkanConstantBuffer.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANCONSTANTBUFFER_H_
#define __RAYNE_VULKANCONSTANTBUFFER_H_

#include "RNVulkan.h"
#include "RNVulkanStateCoordinator.h"

namespace RN
{
	class Renderer;
	class GPUBuffer;

	class VulkanConstantBuffer : public Object
	{
	public:
		VulkanConstantBuffer(size_t size);
		~VulkanConstantBuffer();

		GPUBuffer *Advance(size_t currentFrame, size_t completedFrame);
		GPUBuffer *GetActiveBuffer() const { return _buffers[_bufferIndex]; }

		size_t GetCurrentBufferIndex() const { return _bufferIndex; }

	private:
		std::vector<GPUBuffer*> _buffers;
		std::vector<size_t> _bufferFrames;
		size_t _bufferIndex;
		size_t _size;

		RNDeclareMetaAPI(VulkanConstantBuffer, VKAPI)
	};
}

#endif /* __RAYNE_VULKANCONSTANTBUFFER_H_ */
