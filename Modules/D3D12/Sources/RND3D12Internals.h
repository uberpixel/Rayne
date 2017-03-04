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
			for(D3D12UniformBuffer *buffer : _vertexBuffers)
				delete buffer;
			for(D3D12UniformBuffer *buffer : _fragmentBuffers)
				delete buffer;
		}

		void UpdateRenderingState(Renderer *renderer, const D3D12RenderingState *state)
		{
			if(state == _pipelineState)
				return;

			_pipelineState = state;

			for(D3D12UniformBuffer *buffer : _vertexBuffers)
				delete buffer;
			for(D3D12UniformBuffer *buffer : _fragmentBuffers)
				delete buffer;

			_vertexBuffers.clear();
			_fragmentBuffers.clear();

			for(D3D12RenderingStateArgument *argument : state->vertexArguments)
			{
				switch(argument->type)
				{
				case D3D12RenderingStateArgument::Type::Buffer:
				{
					if(argument->index > 0)
						_vertexBuffers.push_back(new D3D12UniformBuffer(renderer, static_cast<D3D12RenderingStateUniformBufferArgument *>(argument)));
				}

				default:
					break;
				}
			}

			for(D3D12RenderingStateArgument *argument : state->fragmentArguments)
			{
				switch(argument->type)
				{
				case D3D12RenderingStateArgument::Type::Buffer:
				{
					if(argument->index > 0)
						_fragmentBuffers.push_back(new D3D12UniformBuffer(renderer, static_cast<D3D12RenderingStateUniformBufferArgument *>(argument)));
				}

				default:
					break;
				}
			}
		}

		const D3D12RenderingState *_pipelineState;
		std::vector<D3D12UniformBuffer *> _vertexBuffers;
		std::vector<D3D12UniformBuffer *> _fragmentBuffers;
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
