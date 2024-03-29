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

#if RN_PLATFORM_MAC_OS
@implementation RNMetalView
{
	CAMetalLayer *_layer;
};

- (id<CAMetalDrawable>)nextDrawable
{
	return [_layer nextDrawable];
}

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device screen:(RN::Screen*)screen andFormat:(MTLPixelFormat)format
{
	if((self = [super initWithFrame:frameRect]))
	{
		_layer = [CAMetalLayer layer];
		[_layer setDevice:device];
		[_layer setPixelFormat:format];
		[_layer setFramebufferOnly:YES]; //TODO: Should be YES for some extra optimisations, but then it doesn't allow reading from it, which is needed for screenshots.
		
		if(screen && screen->GetNSScreen())
		{
			NSScreen *nsscreen = (NSScreen *)screen->GetNSScreen();
			[_layer setContentsScale:[nsscreen backingScaleFactor]];
		}

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
#endif

#if RN_PLATFORM_IOS
RNMetalLayerContainer::RNMetalLayerContainer(CAMetalLayer *metalLayer)
{
	_metalLayer = metalLayer;
}

id<CAMetalDrawable> RNMetalLayerContainer::GetNextDrawable()
{
	return [_metalLayer nextDrawable];
}

RN::Vector2 RNMetalLayerContainer::GetSize()
{
	return RN::Vector2(_metalLayer.frame.size.width, _metalLayer.frame.size.height);
}
#endif
