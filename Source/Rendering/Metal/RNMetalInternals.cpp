//
//  RNMetalInternals.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalInternals.h"

@implementation RNMetalView
	{
		CAMetalLayer *_layer;
	};

- (id<CAMetalDrawable>)nextDrawable
{
	return [_layer nextDrawable];
}

- (instancetype)initWithFrame:(NSRect)frameRect andDevice:(id<MTLDevice>)device
{
	if((self = [super initWithFrame:frameRect]))
	{
		_layer = [CAMetalLayer layer];
		[_layer setDevice:device];
		[_layer setPixelFormat:MTLPixelFormatBGRA8Unorm];
		[_layer setFramebufferOnly:YES];

		[self setWantsBestResolutionOpenGLSurface:YES];
		[self setWantsLayer:YES];
		[self setLayer:_layer];
	}

	return self;
}

@end

