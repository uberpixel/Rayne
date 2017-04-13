//
//  RNMetalInternals.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalInternals.h"
#include "RNMetalTexture.h"
#include "RNMetalFramebuffer.h"

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
		[_layer setPixelFormat:MTLPixelFormatBGRA8Unorm_sRGB];
		[_layer setFramebufferOnly:YES];
		[_layer setContentsScale:2.0];	//TODO: Use device contentScale

		[self setWantsLayer:YES];
		[self setLayer:_layer];
	}

	return self;
}

- (CGSize)getSize
{
	//TODO: Use layer contentScale
	return CGSizeMake(self.frame.size.width * 2.0f, self.frame.size.height * 2.0f);
}

@end

@implementation RNMetalWindow

- (BOOL)windowShouldClose:(id)sender
{
	return NO;
}

- (void)keyDown:(NSEvent *)theEvent
{}
- (void)keyUp:(NSEvent *)theEvent
{}
- (void)mouseMoved:(NSEvent *)theEvent
{}
- (void)mouseDown:(NSEvent *)theEvent
{}
- (void)rightMouseDown:(NSEvent *)theEvent
{}
- (void)otherMouseDown:(NSEvent *)theEvent
{}
- (void)mouseDragged:(NSEvent *)theEvent
{}
- (void)rightMouseDragged:(NSEvent *)theEvent
{}
- (void)otherMouseDragged:(NSEvent *)theEvent
{}
- (void)mouseUp:(NSEvent *)theEvent
{}
- (void)rightMouseUp:(NSEvent *)theEvent
{}
- (void)otherMouseUp:(NSEvent *)theEvent
{}
- (void)scrollWheel:(NSEvent *)theEvent
{}


- (BOOL)canBecomeKeyWindow
{
	return YES;
}
- (BOOL)canBecomeMainWindow
{
	return YES;
}

@end
