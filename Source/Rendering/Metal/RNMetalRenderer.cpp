//
//  RNMetalRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../../Base/RNBaseInternal.h"
#include "../../Objects/RNNumber.h"
#include "RNMetalRenderer.h"
#include "RNMetalInternals.h"
#include "RNMetalShaderLibrary.h"
#include "RNMetalGPUBuffer.h"
#include "RNMetalTexture.h"

#include "RNMetalShaders.metal"

namespace RN
{
	RNDefineMeta(MetalRenderer, Renderer)

	MTLPixelFormat textureFormat[] = {
		MTLPixelFormatInvalid,
		MTLPixelFormatBGRA8Unorm,
		MTLPixelFormatRGB10A2Unorm,
		MTLPixelFormatR8Unorm,
		MTLPixelFormatRG8Unorm,
		MTLPixelFormatR16Float,
		MTLPixelFormatRG16Float,
		MTLPixelFormatRGBA16Float,
		MTLPixelFormatR32Float,
		MTLPixelFormatRG32Float,
		MTLPixelFormatRGBA32Float,
		MTLPixelFormatDepth24Unorm_Stencil8,
		MTLPixelFormatDepth32Float,
		MTLPixelFormatStencil8,
		MTLPixelFormatDepth24Unorm_Stencil8,
		MTLPixelFormatDepth32Float_Stencil8
	};

	MetalRenderer::MetalRenderer() :
		_mainWindow(nullptr)
	{
		_internals->device = nullptr;

		// Check for parameters that need to be activated before interacting with the Metal API
		Dictionary *parameters = GetParameters();
		if(parameters)
		{
			Number *apiValidation = parameters->GetObjectForKey<Number>(RNCSTR("api_validation"));
			if(apiValidation && apiValidation->GetBoolValue())
			{
				char buffer[64];
				strcpy(buffer, "METAL_DEVICE_WRAPPER_TYPE=1");

				putenv(buffer);
			}
		}

		// Actual initialization
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
		_internals->stateCoordinator.SetDevice(_internals->device);

		[devices release];

		NSError *error = nil;
		_internals->defaultLibrary = [_internals->device newLibraryWithSource:[NSString stringWithUTF8String:kRNMetalRendererDefaultShaders] options:nil error:&error];

		if(!_internals->defaultLibrary)
			throw ShaderCompilationException([[error localizedDescription] UTF8String]);
	}

	MetalRenderer::~MetalRenderer()
	{
		[_internals->commandQueue release];
		[_internals->device release];
	}


	Window *MetalRenderer::CreateWindow(const Vector2 &size, Screen *screen)
	{
		RN_ASSERT(!_mainWindow, "Metal renderer only supports one window");

		_mainWindow = new MetalWindow(size, screen, this);
		return _mainWindow;
	}

	Window *MetalRenderer::GetMainWindow()
	{
		return _mainWindow;
	}


	void MetalRenderer::RenderIntoWindow(Window *window, Function &&function)
	{
		_internals->pass.window = static_cast<MetalWindow *>(window);
		_internals->pass.drawable = [_internals->pass.window->_internals->metalView nextDrawable];
		_internals->pass.depthTexture = [_internals->pass.window->_internals->metalView nextDepthBuffer];

		if(_internals->pass.drawable)
		{
			_internals->renderPass.commandBuffer = [[_internals->commandQueue commandBuffer] retain];

			function();

			[_internals->renderPass.commandBuffer presentDrawable:_internals->pass.drawable];
			[_internals->renderPass.commandBuffer commit];
			[_internals->renderPass.commandBuffer release];

			_internals->renderPass.commandBuffer = nil;
			_internals->pass.drawable = nil;
		}
	}
	void MetalRenderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		// Set up
		_internals->renderPass.camera = camera;
		_internals->renderPass.framebuffer = camera->GetFramebuffer();

		const Color &clearColor = camera->GetClearColor();

		MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];
		MTLRenderPassColorAttachmentDescriptor *colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:0];
		[colorAttachment setTexture:[_internals->pass.drawable texture]];
		[colorAttachment setLoadAction:MTLLoadActionClear];
		[colorAttachment setStoreAction:MTLStoreActionStore];
		[colorAttachment setClearColor:MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a)];

		MTLRenderPassDepthAttachmentDescriptor *depthAttachment = [descriptor depthAttachment];
		[depthAttachment setTexture:_internals->pass.depthTexture];
		[depthAttachment setLoadAction:MTLLoadActionClear];
		[depthAttachment setStoreAction:MTLStoreActionStore];

		MTLRenderPassStencilAttachmentDescriptor *stencilAttachment = [descriptor stencilAttachment];
		[stencilAttachment setTexture:_internals->pass.depthTexture];
		[stencilAttachment setLoadAction:MTLLoadActionDontCare];
		[stencilAttachment setStoreAction:MTLStoreActionDontCare];

		_internals->renderPass.renderCommand = [[_internals->renderPass.commandBuffer renderCommandEncoderWithDescriptor:descriptor] retain];
		_internals->renderPass.drawableHead = nullptr;

		[descriptor release];

		_internals->renderPass.viewMatrix = Matrix::WithIdentity();
		_internals->renderPass.inverseViewMatrix = Matrix::WithIdentity().GetInverse();

		Matrix projectionMatrix = Matrix::WithProjectionPerspective(60, 1.3333, 0.01, 1000.0f);

		_internals->renderPass.projectionMatrix = projectionMatrix;
		_internals->renderPass.inverseProjectionMatrix = projectionMatrix.GetInverse();

		_internals->renderPass.projectionViewMatrix = _internals->renderPass.projectionMatrix * _internals->renderPass.viewMatrix;

		function();

		// Clean Up
		size_t i = 0;
		MetalDrawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(i ++, drawable);
			drawable = drawable->_next;
		}

		[_internals->renderPass.renderCommand endEncoding];
		[_internals->renderPass.renderCommand release];
		_internals->renderPass.renderCommand = nil;
	}

	MTLResourceOptions MetalResourceOptionsFromOptions(GPUResource::UsageOptions options)
	{
		switch(options)
		{
			case GPUResource::UsageOptions::ReadWrite:
				return MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeManaged;
			case GPUResource::UsageOptions::WriteOnly:
				return MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeManaged;
			case GPUResource::UsageOptions::Private:
				return  MTLResourceStorageModePrivate;
		}
	}

	GPUBuffer *MetalRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions options)
	{
		MTLResourceOptions resourceOptions = MetalResourceOptionsFromOptions(options);
		id<MTLBuffer> buffer = [_internals->device newBufferWithLength:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new MetalGPUBuffer(buffer));
	}
	GPUBuffer *MetalRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options)
	{
		MTLResourceOptions resourceOptions = MetalResourceOptionsFromOptions(options);
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

	bool MetalRenderer::SupportsTextureFormat(Texture::Format format)
	{
		return true;
	}
	bool MetalRenderer::SupportsDrawMode(DrawMode mode)
	{
		return true;
	}

	Texture *MetalRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		MTLTextureDescriptor *metalDescriptor = [[MTLTextureDescriptor alloc] init];

		metalDescriptor.width = descriptor.width;
		metalDescriptor.height = descriptor.height;
		metalDescriptor.depth = descriptor.depth;
		metalDescriptor.resourceOptions = MetalResourceOptionsFromOptions(descriptor.usageOptions);
		metalDescriptor.mipmapLevelCount = descriptor.mipMaps;
		metalDescriptor.pixelFormat = textureFormat[static_cast<uint32>(descriptor.format)];
		metalDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;

		switch(descriptor.type)
		{
			case Texture::Descriptor::Type::Type1D:
				metalDescriptor.textureType = MTLTextureType1D;
				break;
			case Texture::Descriptor::Type::Type1DArray:
				metalDescriptor.textureType = MTLTextureType1DArray;
				break;
			case Texture::Descriptor::Type::Type2D:
				metalDescriptor.textureType = MTLTextureType2D;
				break;
			case Texture::Descriptor::Type::Type2DArray:
				metalDescriptor.textureType = MTLTextureType2DArray;
				break;
			case Texture::Descriptor::Type::TypeCube:
				metalDescriptor.textureType = MTLTextureTypeCube;
				break;
			case Texture::Descriptor::Type::TypeCubeArray:
				metalDescriptor.textureType = MTLTextureTypeCubeArray;
				break;
			case Texture::Descriptor::Type::Type3D:
				metalDescriptor.textureType = MTLTextureType3D;
				break;
		}

		id<MTLTexture> texture = [_internals->device newTextureWithDescriptor:metalDescriptor];
		[metalDescriptor release];

		return new MetalTexture(texture, descriptor);
	}

	Drawable *MetalRenderer::CreateDrawable()
	{
		MetalDrawable *drawable = new MetalDrawable();
		drawable->_pipelineState = nullptr;
		drawable->_next = nullptr;
		drawable->_prev = nullptr;
		drawable->_uniformsBufferIndex = 0;

		for(size_t i = 0; i < 3; i ++)
			drawable->_uniformBuffers[i] = static_cast<MetalGPUBuffer *>(CreateBufferWithLength(sizeof(Uniforms), GPUResource::UsageOptions::ReadWrite));

		return drawable;
	}

	void MetalRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		MetalDrawable *drawable = static_cast<MetalDrawable *>(tdrawable);

		if(drawable->dirty)
		{
			_lock.Lock();
			id<MTLRenderPipelineState> state = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);

			if(drawable->_pipelineState)
				[(id<MTLRenderPipelineState>)drawable->_pipelineState release];

			drawable->_pipelineState = [state retain];
			_lock.Unlock();

			drawable->dirty = false;
		}

		// Update uniforms
		//if(drawable->_uniformsBufferIndex == 0)
		{
			drawable->_uniformsBufferIndex = (drawable->_uniformsBufferIndex + 1) % 3;
			Uniforms *uniforms = static_cast<Uniforms *>(drawable->_uniformBuffers[drawable->_uniformsBufferIndex]->GetBuffer());

			uniforms->modelViewProjectionMatrix = _internals->renderPass.projectionViewMatrix * drawable->modelMatrix;
			uniforms->modelMatrix = drawable->modelMatrix;

			/*uniforms->inverseModelMatrix = drawable->uniforms.inverseModelMatrix;
			uniforms->viewMatrix = _internals->renderPass.viewMatrix;
			uniforms->inverseViewMatrix = _internals->renderPass.inverseViewMatrix;
			uniforms->projectionMatrix = _internals->renderPass.projectionMatrix;
			uniforms->inverseProjectionMatrix = _internals->renderPass.inverseProjectionMatrix;*/

			drawable->_uniformBuffers[drawable->_uniformsBufferIndex]->Invalidate();
		}

		// Push into the queue
		drawable->_prev = nullptr;

		_lock.Lock();

		drawable->_next = _internals->renderPass.drawableHead;

		if(drawable->_next)
			drawable->_next->_prev = drawable;

		_internals->renderPass.drawableHead = drawable;
		_internals->renderPass.drawableCount ++;

		_lock.Unlock();
	}

	void MetalRenderer::RenderDrawable(size_t index, MetalDrawable *drawable)
	{
		id<MTLRenderCommandEncoder> encoder = _internals->renderPass.renderCommand;

		if(index == 0)
		{
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetVertexBuffer());

			[encoder setRenderPipelineState: drawable->_pipelineState];
			[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(drawable->material)];
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:0];
		}

		MetalGPUBuffer *indexBuffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetIndicesBuffer());

		[encoder setVertexBuffer:(id<MTLBuffer>)drawable->_uniformBuffers[drawable->_uniformsBufferIndex]->_buffer offset:0 atIndex:1];
		[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:drawable->mesh->GetIndicesCount() indexType:MTLIndexTypeUInt16 indexBuffer:(id<MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];
	}
}
