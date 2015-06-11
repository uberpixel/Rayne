//
//  RNMetalRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../../Base/RNBaseInternal.h"
#include "RNMetalRenderer.h"
#include "RNMetalInternals.h"
#include "RNMetalShaderLibrary.h"
#include "RNMetalGPUBuffer.h"

namespace RN
{
	RNDefineMeta(MetalRenderer, Renderer)

	MetalRenderer::MetalRenderer() :
		_mainWindow(nullptr)
	{
		_internals->device = nullptr;

		NSArray *devices = MTLCopyAllDevices();
		NSUInteger count = [devices count];

		for(NSUInteger i = 0; i < count; i ++)
		{
			id<MTLDevice> device = [devices objectAtIndex:i];
			if(![device isLowPower])
			{
				_internals->device = [device retain];
				break;
			}
		}

		RN_ASSERT(_internals->device, "Needs a valid device");

		_internals->commandQueue = [_internals->device newCommandQueue];
		[devices release];
	}

	MetalRenderer::~MetalRenderer()
	{
		[_internals->commandQueue release];
		[_internals->device release];
	}


	Window *MetalRenderer::CreateWindow(const Rect &frame, Screen *screen)
	{
		RN_ASSERT(!_mainWindow, "Metal renderer only supports one window");

		_mainWindow = new MetalWindow(frame, screen, this);
		return _mainWindow;
	}

	Window *MetalRenderer::GetMainWindow()
	{
		return _mainWindow;
	}


	void MetalRenderer::BeginWindow(Window *window)
	{
		_internals->pass.window = static_cast<MetalWindow *>(window);
		_internals->pass.drawable = [_internals->pass.window->_internals->metalView nextDrawable];

		if(_internals->pass.drawable)
		{
			_internals->pass.commandBuffer = [[_internals->commandQueue commandBuffer] retain];

			MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];
			MTLRenderPassColorAttachmentDescriptor * colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:0];
			[colorAttachment setTexture:[_internals->pass.drawable texture]];
			[colorAttachment setLoadAction:MTLLoadActionClear];
			[colorAttachment setClearColor:MTLClearColorMake(0, 0.1, 0.5, 1.0)];

			id<MTLRenderCommandEncoder> command = [_internals->pass.commandBuffer renderCommandEncoderWithDescriptor:descriptor];
			[command endEncoding];

			[descriptor release];
		}
	}

	void MetalRenderer::EndWindow()
	{
		if(_internals->pass.drawable)
		{
			[_internals->pass.commandBuffer presentDrawable:_internals->pass.drawable];
			[_internals->pass.commandBuffer commit];
			[_internals->pass.commandBuffer release];

			_internals->pass.commandBuffer = nil;
			_internals->pass.drawable = nil;
		}
	}

	GPUBuffer *MetalRenderer::CreateBufferWithLength(size_t length, GPUBuffer::Options options)
	{
		MTLResourceOptions resourceOptions;
		switch(options)
		{
			case GPUBuffer::Options::ReadWrite:
				resourceOptions = MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeManaged;
				break;
			case GPUBuffer::Options::WriteOnly:
				resourceOptions = MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeManaged;
				break;
			case GPUBuffer::Options::Private:
				resourceOptions = MTLResourceStorageModePrivate;
				break;
		}

		id<MTLBuffer> buffer = [_internals->device newBufferWithLength:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new MetalGPUBuffer(buffer));
	}
	GPUBuffer *MetalRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUBuffer::Options options)
	{
		MTLResourceOptions resourceOptions;
		switch(options)
		{
			case GPUBuffer::Options::ReadWrite:
				resourceOptions = MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeManaged;
				break;
			case GPUBuffer::Options::WriteOnly:
				resourceOptions = MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeManaged;
				break;
			case GPUBuffer::Options::Private:
				resourceOptions = MTLResourceStorageModePrivate;
				break;
		}

		id<MTLBuffer> buffer = [_internals->device newBufferWithBytes:bytes length:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new MetalGPUBuffer(buffer));
	}

	ShaderLibrary *MetalRenderer::GetShaderLibraryWithFile(const String *file)
	{
		String *source = String::WithContentsOfFile(file, Encoding::UTF8);
		return GetShaderLibraryWithSource(source);
	}
	ShaderLibrary *MetalRenderer::GetShaderLibraryWithSource(const String *source)
	{
		NSError *error = nil;
		id<MTLLibrary> library = [_internals->device newLibraryWithSource:[NSString stringWithUTF8String:source->GetUTF8String()] options:nil error:&error];

		if(!library)
			throw ShaderCompilationException([[error localizedDescription] UTF8String]);

		MetalShaderLibrary *lib = new MetalShaderLibrary(library);
		return lib->Autorelease();
	}
}
