//
//  RNMetalInternals.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALINTERNALS_H__
#define __RAYNE_METALINTERNALS_H__

#include "../../Base/RNBase.h"
#include "../../Base/RNBaseInternal.h"
#include "RNMetalStateCoordinator.h"
#include "RNMetalUniformBuffer.h"

@interface RNMetalView : NSView
- (id<CAMetalDrawable>)nextDrawable;
- (id<MTLTexture>)nextDepthBuffer;
- (instancetype)initWithFrame:(NSRect)frameRect andDevice:(id<MTLDevice>)device;
@end

namespace RN
{
	class MetalWindow;
	class Framebuffer;
	class Camera;
	class MetalGPUBuffer;

	struct MetalWindowPass
	{
		MetalWindow *window;

		id<CAMetalDrawable> drawable;
		id<MTLTexture> depthTexture;
	};

	struct MetalDrawable : public Drawable
	{
		~MetalDrawable()
		{
			for(MetalUniformBuffer *buffer : _vertexBuffers)
				delete buffer;
		}

		void UpdateRenderingState(Renderer *renderer, const MetalRenderingState *state)
		{
			if(state == _pipelineState)
				return;

			_pipelineState = state;

			for(MetalUniformBuffer *buffer : _vertexBuffers)
				delete buffer;

			_vertexBuffers.clear();

			for(MetalRenderingStateArgument *argument : state->vertexArguments)
			{
				switch(argument->type)
				{
					case MetalRenderingStateArgument::Type::Buffer:
					{
						if(argument->index > 0)
							_vertexBuffers.push_back(new MetalUniformBuffer(renderer, static_cast<MetalRenderingStateUniformBufferArgument *>(argument)));
					}

					default:
						break;
				}
			}
		}

		const MetalRenderingState *_pipelineState;
		std::vector<MetalUniformBuffer *> _vertexBuffers;
		MetalDrawable *_next;
		MetalDrawable *_prev;
	};

	struct MetalRenderPass
	{
		Camera *camera;
		Framebuffer *framebuffer;
		id<MTLCommandBuffer> commandBuffer;
		id<MTLRenderCommandEncoder> renderCommand;

		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		MetalDrawable *drawableHead;
		size_t drawableCount;
	};


	struct MetalRendererInternals
	{
		id<MTLDevice> device;
		id<MTLCommandQueue> commandQueue;
		id<MTLLibrary> defaultLibrary;

		MetalWindowPass pass;
		MetalRenderPass renderPass;
		MetalStateCoordinator stateCoordinator;
	};

	struct MetalWindowInternals
	{
		NSWindow *window;
		RNMetalView *metalView;
	};
}

#endif /* __RAYNE_METALINTERNALS_H__ */
