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
		[devices release];

		const float vertexData[] = {
			-1.0, -1.0, 0.0, 0.0,
			1.0, -1.0, 0.0, 0.0,
			-1.0, 1.0, 0.0, 1.0,
			1.0, 1.0, 1.0, 1.0
		};

		NSError *error = nil;
		_internals->defaultLibrary = [_internals->device newLibraryWithSource:[NSString stringWithUTF8String:kRNMetalRendererDefaultShaders] options:nil error:&error];

		if(!_internals->defaultLibrary)
			throw ShaderCompilationException([[error localizedDescription] UTF8String]);

		MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];

		[[[vertexDescriptor attributes] objectAtIndexedSubscript:0] setFormat:MTLVertexFormatFloat2];
		[[[vertexDescriptor attributes] objectAtIndexedSubscript:0] setOffset:0];
		[[[vertexDescriptor attributes] objectAtIndexedSubscript:0] setBufferIndex:0];

		[[[vertexDescriptor attributes] objectAtIndexedSubscript:1] setFormat:MTLVertexFormatFloat2];
		[[[vertexDescriptor attributes] objectAtIndexedSubscript:1] setOffset:2 * sizeof(float)];
		[[[vertexDescriptor attributes] objectAtIndexedSubscript:1] setBufferIndex:0];

		[[[vertexDescriptor layouts] objectAtIndexedSubscript:0] setStride:4 * sizeof(float)];
		[[[vertexDescriptor layouts] objectAtIndexedSubscript:0] setStepFunction:MTLVertexStepFunctionPerVertex];

		MTLRenderPipelineDescriptor *descriptor = [[MTLRenderPipelineDescriptor alloc] init];
		[descriptor setVertexFunction:[_internals->defaultLibrary newFunctionWithName:@"basic_blit_vertex"]];
		[descriptor setFragmentFunction:[_internals->defaultLibrary newFunctionWithName:@"basic_blit_fragment"]];
		[descriptor setVertexDescriptor:vertexDescriptor];
		[[[descriptor colorAttachments] objectAtIndexedSubscript:0] setPixelFormat:MTLPixelFormatBGRA8Unorm];

		_internals->blitState = [_internals->device newRenderPipelineStateWithDescriptor:descriptor error:&error];
		if(!_internals->blitState)
			throw InconsistencyException([[error localizedDescription] UTF8String]);

		_internals->blitVertexBuffer = [_internals->device newBufferWithBytes:vertexData length:sizeof(vertexData) options:MTLResourceStorageModeShared];

		MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
		samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
		samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
		_internals->blitSampler = [_internals->device newSamplerStateWithDescriptor:samplerDescriptor];
		[samplerDescriptor release];
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


	void MetalRenderer::BeginWindow(Window *window)
	{
		_internals->pass.window = static_cast<MetalWindow *>(window);
		_internals->pass.drawable = [_internals->pass.window->_internals->metalView nextDrawable];

		if(_internals->pass.drawable)
		{
			MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];
			MTLRenderPassColorAttachmentDescriptor *colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:0];
			[colorAttachment setTexture:[_internals->pass.drawable texture]];
			[colorAttachment setLoadAction:MTLLoadActionDontCare];

			_internals->pass.commandBuffer = [[_internals->commandQueue commandBuffer] retain];
			_internals->pass.blitCommand = [_internals->pass.commandBuffer renderCommandEncoderWithDescriptor:descriptor];

			[descriptor release];

			[_internals->pass.blitCommand setRenderPipelineState:_internals->blitState];
			[_internals->pass.blitCommand setVertexBuffer:_internals->blitVertexBuffer offset:0 atIndex:0];
		}
	}

	void MetalRenderer::EndWindow()
	{
		if(_internals->pass.drawable)
		{
			[_internals->pass.blitCommand endEncoding];

			[_internals->pass.commandBuffer presentDrawable:_internals->pass.drawable];
			[_internals->pass.commandBuffer commit];
			[_internals->pass.commandBuffer release];

			_internals->pass.commandBuffer = nil;
			_internals->pass.blitCommand = nil;
			_internals->pass.drawable = nil;
		}
	}

	void MetalRenderer::BeginCamera(Camera *camera)
	{
		_internals->renderPass.camera = camera;
		_internals->renderPass.framebuffer = camera->GetFramebuffer();
		_internals->renderPass.commandBuffer = [[_internals->commandQueue commandBuffer] retain];

		MetalTexture *colorTexture = static_cast<MetalTexture *>(_internals->renderPass.framebuffer->GetColorTexture());
		const Color &clearColor = camera->GetClearColor();

		MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];
		MTLRenderPassColorAttachmentDescriptor *colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:0];
		[colorAttachment setTexture:(id<MTLTexture>)colorTexture->_texture];
		[colorAttachment setLoadAction:MTLLoadActionClear];
		[colorAttachment setClearColor:MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a)];

		id<MTLRenderCommandEncoder> command = [_internals->renderPass.commandBuffer renderCommandEncoderWithDescriptor:descriptor];
		[command endEncoding];

		[descriptor release];
	}

	void MetalRenderer::EndCamera()
	{
		[_internals->renderPass.commandBuffer commit];
		[_internals->renderPass.commandBuffer release];

		Framebuffer *framebuffer = _internals->renderPass.framebuffer;
		MetalTexture *colorTexture = static_cast<MetalTexture *>(framebuffer->GetColorTexture());

		[_internals->pass.blitCommand setFragmentSamplerState:_internals->blitSampler atIndex:0];
		[_internals->pass.blitCommand setFragmentTexture:(id<MTLTexture>)colorTexture->_texture atIndex:0];
		[_internals->pass.blitCommand drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

		_internals->renderPass.commandBuffer = nil;
		_internals->renderPass.framebuffer = nullptr;
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
}
