//
//  RNMetalRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalRenderer.h"
#include "RNMetalInternals.h"
#include "RNMetalShaderLibrary.h"
#include "RNMetalGPUBuffer.h"
#include "RNMetalTexture.h"
#include "RNMetalUniformBuffer.h"

#include "RNMetalShaders.metal"

namespace RN
{
	RNDefineMeta(MetalRenderer, Renderer)


	MetalRenderer::MetalRenderer(const Dictionary *parameters) :
		_mainWindow(nullptr),
		_mipMapTextures(new Set())
	{
		_internals->device = nullptr;

		// Check for parameters that need to be activated before interacting with the Metal API
		if(parameters)
		{
			Number *apiValidation = const_cast<Dictionary *>(parameters)->GetObjectForKey<Number>(RNCSTR("api_validation"));
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

		// Texture format look ups
		_textureFormatLookup = new Dictionary();

#define TextureFormat(name, metal) \
		_textureFormatLookup->SetObjectForKey(Number::WithUint32(metal), RNCSTR(#name))

		TextureFormat(RGBA8888, MTLPixelFormatBGRA8Unorm);
		TextureFormat(RGB10A2, MTLPixelFormatRGB10A2Unorm);
		TextureFormat(R8, MTLPixelFormatR8Unorm);
		TextureFormat(RG88, MTLPixelFormatRG8Unorm);
		TextureFormat(R16F, MTLPixelFormatR16Float);
		TextureFormat(RG16F, MTLPixelFormatRG16Float);
		TextureFormat(RGBA16F, MTLPixelFormatRGBA16Float);
		TextureFormat(R32F, MTLPixelFormatR32Float);
		TextureFormat(RG32F, MTLPixelFormatRG32Float);
		TextureFormat(RGBA32F, MTLPixelFormatRGBA32Float);
		TextureFormat(Depth24I, MTLPixelFormatDepth24Unorm_Stencil8);
		TextureFormat(Depth32F, MTLPixelFormatDepth32Float);
		TextureFormat(Stencil8, MTLPixelFormatStencil8);
		TextureFormat(Depth24Stencil8, MTLPixelFormatDepth24Unorm_Stencil8);
		TextureFormat(Depth32FStencil8, MTLPixelFormatDepth32Float_Stencil8);

#undef TextureFormat
	}

	MetalRenderer::~MetalRenderer()
	{
		[_internals->commandQueue release];
		[_internals->device release];

		_mipMapTextures->Release();
		_textureFormatLookup->Release();
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


	void MetalRenderer::CreateMipMapForeTexture(MetalTexture *texture)
	{
		_lock.Lock();
		_mipMapTextures->AddObject(texture);
		_lock.Unlock();
	}

	void MetalRenderer::CreateMipMaps()
	{
		if(_mipMapTextures->GetCount() == 0)
			return;

		id<MTLCommandBuffer> commandBuffer = [_internals->commandQueue commandBuffer];

		_mipMapTextures->Enumerate<MetalTexture>([&](MetalTexture *texture, bool &stop) {

			id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
			[commandEncoder generateMipmapsForTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture()];
			[commandEncoder endEncoding];
		});

		[commandBuffer commit];
		[commandBuffer waitUntilCompleted];

		_mipMapTextures->RemoveAllObjects();
	}


	void MetalRenderer::RenderIntoWindow(Window *window, Function &&function)
	{
		_internals->pass.window = static_cast<MetalWindow *>(window);
		_internals->pass.drawable = [_internals->pass.window->_internals->metalView nextDrawable];
		_internals->pass.depthTexture = [_internals->pass.window->_internals->metalView nextDepthBuffer];

		if(_internals->pass.drawable)
		{
			CreateMipMaps();

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
		_internals->renderPass.activeState = nullptr;
		_internals->renderPass.drawableHead = nullptr;

		[descriptor release];

		_internals->renderPass.viewMatrix = camera->GetViewMatrix();
		_internals->renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		_internals->renderPass.projectionMatrix = camera->GetProjectionMatrix();
		_internals->renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		_internals->renderPass.projectionViewMatrix = _internals->renderPass.projectionMatrix * _internals->renderPass.viewMatrix;

		function();

		// Clean Up
		MetalDrawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(drawable);
			drawable = drawable->_next;
		}

		[_internals->renderPass.renderCommand endEncoding];
		[_internals->renderPass.renderCommand release];
		_internals->renderPass.renderCommand = nil;

		_internals->renderPass.activeState = nullptr;
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

	const String *MetalRenderer::GetTextureFormatName(const Texture::Format format)
	{
#define TextureFormat(name) \
		case Texture::Format::name: \
			return RNCSTR(#name) \

		switch(format)
		{
			TextureFormat(RGBA8888);
			TextureFormat(RGB10A2);
			TextureFormat(R8);
			TextureFormat(RG88);
			TextureFormat(R16F);
			TextureFormat(RG16F);
			TextureFormat(RGBA16F);
			TextureFormat(R32F);
			TextureFormat(RG32F);
			TextureFormat(RGBA32F);
			TextureFormat(Depth24I);
			TextureFormat(Depth32F);
			TextureFormat(Stencil8);
			TextureFormat(Depth24Stencil8);
			TextureFormat(Depth32FStencil8);

			default:
				return nullptr;
		}

#undef TextureFormat
	}

	Texture *MetalRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		String *formatName = const_cast<String *>(descriptor.GetFormat());
		if(!formatName)
			throw InvalidTextureFormatException("Texture Format is NULL!");

		Number *format = _textureFormatLookup->GetObjectForKey<Number>(formatName);
		if(!format)
			throw InvalidTextureFormatException(RNSTR("Unsupported texture format '" << format << "'"));

		MTLTextureDescriptor *metalDescriptor = [[MTLTextureDescriptor alloc] init];

		metalDescriptor.width = descriptor.width;
		metalDescriptor.height = descriptor.height;
		metalDescriptor.depth = descriptor.depth;
		metalDescriptor.resourceOptions = MetalResourceOptionsFromOptions(descriptor.usageOptions);
		metalDescriptor.mipmapLevelCount = descriptor.mipMaps;
		metalDescriptor.pixelFormat = static_cast<MTLPixelFormat>(format->GetUint32Value());

		MTLTextureUsage usage = 0;

		if(descriptor.usageHint & Texture::Descriptor::UsageHint::ShaderRead)
			usage |= MTLTextureUsageShaderRead;
		if(descriptor.usageHint & Texture::Descriptor::UsageHint::ShaderWrite)
			usage |= MTLTextureUsageShaderWrite;
		if(descriptor.usageHint & Texture::Descriptor::UsageHint::RenderTarget)
			usage |= MTLTextureUsageRenderTarget;

		metalDescriptor.usage = usage;


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

		return new MetalTexture(this, &_internals->stateCoordinator, texture, descriptor);
	}

	Drawable *MetalRenderer::CreateDrawable()
	{
		MetalDrawable *drawable = new MetalDrawable();
		drawable->_pipelineState = nullptr;
		drawable->_next = nullptr;
		drawable->_prev = nullptr;

		return drawable;
	}

	void MetalRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		MetalDrawable *drawable = static_cast<MetalDrawable *>(tdrawable);

		if(drawable->dirty)
		{
			_lock.Lock();
			const MetalRenderingState *state = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);
			_lock.Unlock();

			drawable->UpdateRenderingState(this, state);
			drawable->dirty = false;
		}

		// Update uniforms
		{
			for(MetalUniformBuffer *uniformBuffer : drawable->_vertexBuffers)
			{
				GPUBuffer *gpuBuffer = uniformBuffer->Advance();
				uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());

				const MetalUniformBuffer::Member *member;

				// Matrices
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelMatrix)))
				{
					std::memcpy(buffer + member->GetOffset(), drawable->modelMatrix.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelViewMatrix)))
				{
					Matrix result = _internals->renderPass.viewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelViewProjectionMatrix)))
				{
					Matrix result = _internals->renderPass.projectionViewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ViewMatrix)))
				{
					std::memcpy(buffer + member->GetOffset(), _internals->renderPass.viewMatrix.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ViewProjectionMatrix)))
				{
					std::memcpy(buffer + member->GetOffset(), _internals->renderPass.projectionViewMatrix.m, sizeof(Matrix));
				}

				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseModelMatrix)))
				{
					std::memcpy(buffer + member->GetOffset(), drawable->inverseModelMatrix.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseModelViewMatrix)))
				{
					Matrix result = _internals->renderPass.inverseViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseModelViewProjectionMatrix)))
				{
					Matrix result = _internals->renderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseViewMatrix)))
				{
					std::memcpy(buffer + member->GetOffset(), _internals->renderPass.inverseViewMatrix.m, sizeof(Matrix));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::InverseViewProjectionMatrix)))
				{
					std::memcpy(buffer + member->GetOffset(), _internals->renderPass.inverseProjectionViewMatrix.m, sizeof(Matrix));
				}

				// Color
				Material *material = drawable->material;

				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::AmbientColor)))
				{
					std::memcpy(buffer + member->GetOffset(), &material->GetAmbientColor().r, sizeof(Color));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::DiffuseColor)))
				{
					std::memcpy(buffer + member->GetOffset(), &material->GetDiffuseColor().r, sizeof(Color));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::SpecularColor)))
				{
					std::memcpy(buffer + member->GetOffset(), &material->GetSpecularColor().r, sizeof(Color));
				}
				if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::EmissiveColor)))
				{
					std::memcpy(buffer + member->GetOffset(), &material->GetEmissiveColor().r, sizeof(Color));
				}

				gpuBuffer->Invalidate();
			}
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

	void MetalRenderer::RenderDrawable(MetalDrawable *drawable)
	{
		id<MTLRenderCommandEncoder> encoder = _internals->renderPass.renderCommand;

		if(_internals->renderPass.activeState != drawable->_pipelineState)
		{
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetVertexBuffer());

			[encoder setRenderPipelineState: drawable->_pipelineState->state];
			[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(drawable->material)];
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:0];

			_internals->renderPass.activeState = drawable->_pipelineState;
		}

		MetalGPUBuffer *indexBuffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetIndicesBuffer());

		for(MetalUniformBuffer *uniformBuffer : drawable->_vertexBuffers)
		{
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBuffer->GetActiveBuffer());
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
		}

		const Array *textures = drawable->material->GetTextures();
		size_t count = textures->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			MetalTexture *texture = textures->GetObjectAtIndex<MetalTexture>(i);

			[encoder setFragmentSamplerState:(id<MTLSamplerState>)texture->__GetUnderlyingSampler() atIndex:i];
			[encoder setFragmentTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture() atIndex:i];
		}

		[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:drawable->mesh->GetIndicesCount() indexType:MTLIndexTypeUInt16 indexBuffer:(id<MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];
	}
}
