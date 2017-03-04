//
//  RNVulkanInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINTERNALS_H__
#define __RAYNE_VULKANINTERNALS_H__

#include "RND3D12Renderer.h"
#include "RND3D12UniformBuffer.h"
#include "RND3D12StateCoordinator.h"

namespace RN
{
	struct D3D12Drawable : public Drawable
	{
		~D3D12Drawable()
		{

		}

		void UpdateRenderingState(Renderer *renderer, const D3D12PipelineState *pipelineState, D3D12UniformState *uniformState)
		{
			if(pipelineState == _pipelineState && uniformState == _uniformState)
				return;

			_pipelineState = pipelineState;
			_uniformState = uniformState;
		}

		const D3D12PipelineState *_pipelineState;
		D3D12UniformState *_uniformState;

		D3D12Drawable *_next;
		D3D12Drawable *_prev;
	};

	struct D3D12RenderPass
	{
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		D3D12Drawable *drawableHead;
		size_t drawableCount;
	};

	struct D3D12RendererInternals
	{
		D3D12RenderPass renderPass;
		D3D12StateCoordinator stateCoordinator;
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */
