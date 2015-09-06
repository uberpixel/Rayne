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
		id<MTLRenderPipelineState> _pipelineState;
		MetalGPUBuffer *_uniformBuffers[3];
		size_t _uniformsBufferIndex;
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
