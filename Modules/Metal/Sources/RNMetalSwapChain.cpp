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

	MetalSwapChain::MetalSwapChain(const Vector2 size, id<MTLDevice> device, const Window::SwapChainDescriptor &descriptor) : _drawable(nullptr)
	{
		_metalView = [[RNMetalView alloc] initWithFrame:NSMakeRect(0, 0, size.x, size.y) device:device andFormat:MetalTexture::PixelFormatForTextureFormat(descriptor.colorFormat)];
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

	void MetalSwapChain::AcquireBackBuffer()
	{
		_drawable = nil;//[_metalView nextDrawable];
	}
	void MetalSwapChain::Prepare()
	{

	}
	void MetalSwapChain::Finalize()
	{

	}
	void MetalSwapChain::PresentBackBuffer(id<MTLCommandBuffer> commandBuffer)
	{
		//[commandBuffer presentDrawable:_drawable];
	}
	
	void MetalSwapChain::PostPresent(id<MTLCommandBuffer> commandBuffer)
	{
		
	}

	id MetalSwapChain::GetMTLTexture() const
	{
		return [_drawable texture];
	}
}
