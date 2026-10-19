//
//  RNMetalWindow.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalWindow.h"
#include "RNMetalInternals.h"
#include "RNMetalRenderer.h"
#include "RNMetalSwapChain.h"
#include "RNMetalTextureInfo.h"

namespace RN
{
	RNDefineMeta(MetalWindow, Window)

	MetalWindow::MetalWindow(const Vector2 &size, Screen *screen, MetalRenderer *renderer, const Window::SwapChainDescriptor &descriptor) :
		Window(screen),
		_renderer(renderer)
	{
#if RN_PLATFORM_MAC_OS
		_internals->window = [[RNMetalWindow alloc] initWithContentRect:NSMakeRect(0, 0, size.x, size.y) styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
		[_internals->window setBackgroundColor:[NSColor blackColor]];
		[_internals->window setIgnoresMouseEvents:NO];

		_swapChain = new MetalSwapChain(size, renderer, renderer->_internals->device, screen, descriptor);

		[_internals->window setContentView:_swapChain->_metalView];
		
		NSScreen *nsscreen = (NSScreen *)screen->GetNSScreen();
		if(descriptor.wantsFullscreen)
		{
			NSDictionary *fullscreenOptions = nil;//@{NSFullScreenModeApplicationPresentationOptions : @(NSApplicationPresentationHideMenuBar | NSApplicationPresentationHideDock | NSApplicationPresentationFullScreen | NSApplicationPresentationDisableAppleMenu)};
			[_swapChain->_metalView enterFullScreenMode:nsscreen withOptions:fullscreenOptions];
			_swapChain->_metalView.wantsLayer = YES;
		}

		NSRect frame = [_internals->window frame];
		CGFloat xPos = (NSWidth([nsscreen frame]) - NSWidth(frame)) * 0.5;
		CGFloat yPos = (NSHeight([nsscreen frame]) - NSHeight(frame)) * 0.5;
		[_internals->window setFrame:NSMakeRect(xPos, yPos, NSWidth(frame), NSHeight(frame)) display:YES];
		[_swapChain->_metalView setWindowOwner:this];
#endif
		
#if RN_PLATFORM_IOS
		CAMetalLayer *metalLayer = (CAMetalLayer*)Kernel::GetSharedInstance()->GetMetalLayer();
		metalLayer.device = renderer->_internals->device;
		[metalLayer setPixelFormat:MetalTextureInfo::GetPixelFormat(descriptor.colorFormat)];
		[metalLayer setFramebufferOnly:YES];
		_internals->metalLayerContainer = new RNMetalLayerContainer(metalLayer);

		// Create the swap chain
		_swapChain = new MetalSwapChain(_internals->metalLayerContainer->GetSize(), renderer, _internals->metalLayerContainer, descriptor);
#endif
	}

	MetalWindow::~MetalWindow()
	{
#if RN_PLATFORM_MAC_OS
		[_swapChain->_metalView setWindowOwner:nullptr];
#endif
	}

	void MetalWindow::UpdateSize()
	{
#if RN_PLATFORM_MAC_OS
		CGSize backingSize = [_swapChain->_metalView getSize];
		if(backingSize.width < 1.0 || backingSize.height < 1.0) return;

		Vector2 size(backingSize.width, backingSize.height);
		if(_swapChain->_size == size)
		{
			[_swapChain->_metalView setDrawableSize:backingSize];
			return;
		}

		// Queued frames must acquire their old-sized drawable before the layer changes.
		_renderer->SynchronizeRenderThread();
		[_swapChain->_metalView setDrawableSize:backingSize];
		_swapChain->_size = size;
		_swapChain->_framebuffer->Downcast<MetalFramebuffer>()->DidUpdateSwapChain(size, _swapChain->_descriptor.colorFormat, _swapChain->_descriptor.depthStencilFormat);
		NotificationManager::GetSharedInstance()->PostNotification(kRNWindowDidChangeSize, this);
#endif
	}

	void MetalWindow::SetTitle(const String *title)
	{
#if RN_PLATFORM_MAC_OS
		[_internals->window setTitle:[NSString stringWithUTF8String:title->GetUTF8String()]];
#endif
	}

	Screen *MetalWindow::GetScreen()
	{
		Array *screens = Screen::GetScreens();
		
#if RN_PLATFORM_MAC_OS
		NSScreen *nsscreen = [_internals->window screen];

		Screen *result = nullptr;

		screens->Enumerate<Screen>([&](Screen *screen, size_t index, bool &stop) {

			if(screen->GetNSScreen() == nsscreen)
			{
				result = screen;
				stop = true;
			}

		});
		
		RN_ASSERT(result, "Result must not be NULL, something broke internally");
		return result;
#else
		return screens->GetFirstObject<Screen>();
#endif
	}

	void MetalWindow::Show()
	{
#if RN_PLATFORM_MAC_OS
		[_internals->window makeKeyAndOrderFront:nil];
#endif
	}

	void MetalWindow::Hide()
	{
#if RN_PLATFORM_MAC_OS
		[_internals->window orderOut:nil];
#endif
	}

	void MetalWindow::SetFullscreen(bool fullscreen)
	{
#if RN_PLATFORM_MAC_OS
		[_internals->window toggleFullScreen:nil];
#endif
	}

	Vector2 MetalWindow::GetSize() const
	{
		return _swapChain->GetSize();
	}

	Framebuffer *MetalWindow::GetFramebuffer() const
	{
		return _swapChain->GetFramebuffer();
	}
	
	const Window::SwapChainDescriptor &MetalWindow::GetSwapChainDescriptor() const
	{
		return _swapChain->GetSwapChainDescriptor();
	}
	
	uint64 MetalWindow::GetWindowHandle() const
	{
#if RN_PLATFORM_MAC_OS
		return reinterpret_cast<uint64>(_internals->window);
#else
		return 0;
#endif
	}
}
