//
//  RNMetalSwapChain.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalSwapChain.h"
#include "RNMetalInternals.h"
#include "RNMetalTexture.h"

namespace RN
{
	RNDefineMeta(MetalSwapChain, Object)

	MetalSwapChain::MetalSwapChain(const Vector2 size, id<MTLDevice> device, Screen *screen, const Window::SwapChainDescriptor &descriptor) : _frameIndex(0), _frameDivider(1), _drawable(nullptr)
	{
		_metalView = [[RNMetalView alloc] initWithFrame:NSMakeRect(0, 0, size.x, size.y) device:device screen:screen andFormat:MetalTexture::PixelFormatForTextureFormat(descriptor.colorFormat)];
		CGSize realSize = [_metalView getSize];
		_size = Vector2(realSize.width, realSize.height);

		_framebuffer = new MetalFramebuffer(_size, this, descriptor.colorFormat, descriptor.depthStencilFormat);
	}

	MetalSwapChain::~MetalSwapChain()
	{

	}

	Vector2 MetalSwapChain::GetSize() const
	{
		return _size;
	}
	
	void MetalSwapChain::SetFrameDivider(uint8 divider)
	{
		_frameDivider = divider;
	}

	void MetalSwapChain::AcquireBackBuffer()
	{
		_frameIndex += 1;
		
		if(_frameDivider && _frameIndex%_frameDivider == 0)
		{
			_drawable = [_metalView nextDrawable];
		}
		else
		{
			_drawable = nullptr;
		}
	}
	void MetalSwapChain::Prepare()
	{

	}
	void MetalSwapChain::Finalize()
	{

	}
	void MetalSwapChain::PresentBackBuffer(id<MTLCommandBuffer> commandBuffer)
	{
		if(_drawable)
			[commandBuffer presentDrawable:_drawable];
	}
	
	void MetalSwapChain::PostPresent(id<MTLCommandBuffer> commandBuffer)
	{
		
	}

	id MetalSwapChain::GetMTLTexture() const
	{
		if(_drawable)
			return [_drawable texture];
		else
			return nil;
	}
}
