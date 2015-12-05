//
//  RND3D12Internals.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Internals.h"
#include "RND3D12Texture.h"

/*@implementation RND3D12View
	{
		CAD3D12Layer *_layer;
		RN::Framebuffer *_depthbuffer[3];
		size_t _depthbufferIndex;
	};

- (id<MTLTexture>)nextDepthBuffer
{
	RN::D3D12Texture *texture = static_cast<RN::D3D12Texture *>(_depthbuffer[_depthbufferIndex]->GetDepthTexture());
	return (id<MTLTexture>)texture->__GetUnderlyingTexture();
}

- (id<CAD3D12Drawable>)nextDrawable
{
	_depthbufferIndex = (_depthbufferIndex + 1) % 3;
	return [_layer nextDrawable];
}

- (instancetype)initWithFrame:(NSRect)frameRect andDevice:(id<MTLDevice>)device
{
	if((self = [super initWithFrame:frameRect]))
	{
		_layer = [CAD3D12Layer layer];
		[_layer setDevice:device];
		[_layer setPixelFormat:MTLPixelFormatBGRA8Unorm];
		[_layer setFramebufferOnly:YES];
		[_layer setContentsScale:2.0];

		[self setWantsLayer:YES];
		[self setLayer:_layer];

		for(size_t i = 0; i < 3; i ++)
		{
			_depthbuffer[i] = new RN::Framebuffer(RN::Vector2(frameRect.size.width * 2.0f, frameRect.size.height * 2.0f), RN::Framebuffer::Options::PrivateStorage, RN::Texture::Format::Invalid, RN::Texture::Format::Depth24I, RN::Texture::Format::Invalid);
		}
	}

	return self;
}

@end

@implementation RND3D12Window

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

@end*/
