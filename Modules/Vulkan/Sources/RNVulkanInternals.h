//
//  RNVulkanInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINTERNALS_H__
#define __RAYNE_VULKANINTERNALS_H__

#include "RNVulkan.h"
#include "RNVulkanStateCoordinator.h"
#include "RNVulkanRenderer.h"

namespace RN
{
	struct VulkanDrawable : public Drawable
	{
		~VulkanDrawable()
		{

		}

		void UpdateRenderingState(Renderer *renderer, const VulkanPipelineState *pipelineState, VulkanUniformState *uniformState)
		{
			if(pipelineState == _pipelineState && uniformState == _uniformState)
				return;

			_pipelineState = pipelineState;
			_uniformState = uniformState;
		}

		const VulkanPipelineState *_pipelineState;
		VulkanUniformState *_uniformState;

		VulkanDrawable *_next;
		VulkanDrawable *_prev;
	};

	struct VulkanRenderPass
	{
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		VulkanDrawable *drawableHead;
		size_t drawableCount;
	};

	class VulkanCommandBuffer : public Object
	{
	public:
		friend RN::VulkanRenderer;

		~VulkanCommandBuffer();

		void Begin();
		void End();

		VkCommandBuffer GetCommandBuffer() const {return _commandBuffer;}

	protected:
		VulkanCommandBuffer(VkDevice device, VkCommandPool pool);

	private:
		VkCommandBuffer _commandBuffer;
		VkDevice _device;
		VkCommandPool _pool;

		RNDeclareMetaAPI(VulkanCommandBuffer, VKAPI)
	};

	class VulkanCommandBufferWithCallback : public VulkanCommandBuffer
	{
	public:
		friend RN::VulkanRenderer;

		~VulkanCommandBufferWithCallback();
		void SetFinishedCallback(std::function<void()> callback);

	protected:
		VulkanCommandBufferWithCallback(VkDevice device, VkCommandPool pool);

	private:
		std::function<void()> _finishedCallback;

		RNDeclareMetaAPI(VulkanCommandBufferWithCallback, VKAPI)
	};

	struct VulkanRendererInternals
	{
		VulkanRenderPass renderPass;
		VulkanStateCoordinator stateCoordinator;
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */
