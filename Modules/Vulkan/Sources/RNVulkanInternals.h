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

	struct VulkanRendererInternals
	{
		VulkanRenderPass renderPass;
		VulkanStateCoordinator stateCoordinator;
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */
