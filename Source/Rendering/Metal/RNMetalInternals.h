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

@interface RNMetalView : NSView
- (id<CAMetalDrawable>)nextDrawable;
- (instancetype)initWithFrame:(NSRect)frameRect andDevice:(id<MTLDevice>)device;
@end

namespace RN
{
	class MetalWindow;

	struct MetalWindowPass
	{
		MetalWindow *window;
		id<CAMetalDrawable> drawable;
		id<MTLCommandBuffer> commandBuffer;
	};

	struct MetalRendererInternals
	{
		id<MTLDevice> device;
		id<MTLCommandQueue> commandQueue;
		MetalWindowPass pass;
	};

	struct MetalWindowInternals
	{
		NSWindow *window;
		RNMetalView *metalView;
	};
}

#endif /* __RAYNE_METALINTERNALS_H__ */
