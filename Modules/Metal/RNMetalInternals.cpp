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

@implementation RNMetalView
	{
		CAMetalLayer *_layer;
		RN::Framebuffer *_depthbuffer[3];
		size_t _depthbufferIndex;
	};

- (id<MTLTexture>)nextDepthBuffer
{
	RN::MetalTexture *texture = static_cast<RN::MetalTexture *>(_depthbuffer[_depthbufferIndex]->GetDepthTexture());
	return (id<MTLTexture>)texture->__GetUnderlyingTexture();
}

- (id<CAMetalDrawable>)nextDrawable
{
	_depthbufferIndex = (_depthbufferIndex + 1) % 3;
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

		for(size_t i = 0; i < 3; i ++)
		{
			_depthbuffer[i] = new RN::Framebuffer(RN::Vector2(frameRect.size.width, frameRect.size.height), RN::Framebuffer::Options::PrivateStorage, RN::Texture::Format::Invalid, RN::Texture::Format::Depth24I, RN::Texture::Format::Invalid);
		}
	}

	return self;
}

@end

