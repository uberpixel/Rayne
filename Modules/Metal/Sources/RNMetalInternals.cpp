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
#include "RNMetalSwapChain.h"
#include "RNMetalWindow.h"
#include "../../../Source/Rendering/RNFrameSubmissionPruner.h"

#if RN_PLATFORM_MAC_OS
@implementation RNMetalView
{
	CAMetalLayer *_layer;
	RN::MetalWindow *_windowOwner;
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
		[self setDrawableSize:[self getSize]];
	}

	return self;
}

- (CGSize)getSize
{
	CGSize size = self.window ? [self convertRectToBacking:self.bounds].size : CGSizeMake(self.bounds.size.width * _layer.contentsScale, self.bounds.size.height * _layer.contentsScale);
	return CGSizeMake(std::ceil(size.width), std::ceil(size.height));
}

- (void)setDrawableSize:(CGSize)size
{
	if(self.window) _layer.contentsScale = self.window.backingScaleFactor;
	_layer.drawableSize = size;
}

- (void)setWindowOwner:(RN::MetalWindow *)window
{
	_windowOwner = window;
	if(_windowOwner) _windowOwner->UpdateSize();
}

- (void)setFrameSize:(NSSize)size
{
	[super setFrameSize:size];
	if(_windowOwner) _windowOwner->UpdateSize();
}

- (void)viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	if(_windowOwner) _windowOwner->UpdateSize();
}

- (void)viewDidChangeBackingProperties
{
	[super viewDidChangeBackingProperties];
	if(_windowOwner) _windowOwner->UpdateSize();
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

namespace RN
{
	bool FrameSubmissionPassUsesFramePass(const MetalRenderPass &metalRenderPass, FramePass *framePass)
	{
		return metalRenderPass.renderPass == framePass || metalRenderPass.computePass == framePass;
	}

	template<class Pruner>
	void FrameSubmissionAddFramePassDependencies(Pruner &pruner, size_t consumerIndex, MetalRenderPass &metalRenderPass)
	{
		pruner.AddExplicitFramePassDependencies(consumerIndex, metalRenderPass.renderPass);
		pruner.AddExplicitFramePassDependencies(consumerIndex, metalRenderPass.computePass);
	}

	void MetalFrameSubmission::AddSwapChain(MetalSwapChain *swapChain)
	{
		if(!swapChain) return;

		for(MetalSwapChain *existingSwapChain : swapChains)
		{
			if(existingSwapChain == swapChain) return;
		}

		RenderFramePresentationState *presentationState = swapChain->TakeRenderFramePresentationState(renderFrame.GetFrameID());
		renderFrame.AddPresentationState(presentationState);
		if(swapChain->ShouldRenderBackBuffer())
			swapChains.push_back(swapChain);
	}

	void MetalFrameSubmission::PruneSkippedRenderPasses()
	{
		FrameSubmissionPruner<MetalFrameSubmission> pruner(*this);
		pruner.Prune();
	}
}
