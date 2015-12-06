//
//  RND3D12Renderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Renderer.h"
#include "RND3D12Internals.h"
#include "RND3D12ShaderLibrary.h"
#include "RND3D12GPUBuffer.h"
#include "RND3D12Texture.h"
#include "RND3D12UniformBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12Renderer, Renderer)

	D3D12Renderer::D3D12Renderer(const Dictionary *parameters) :
		_mainWindow(nullptr),
		_mipMapTextures(new Set())
	{
/*		_internals->device = nullptr;

		// Actual initialization
		NSArray *devices = MTLCopyAllDevices();
		NSUInteger count = [devices count];

		for(NSUInteger i = 0; i < count; i ++)
		{
			id<MTLDevice> device = [devices objectAtIndex:i];
			if(![device isLowPower] && ![device isHeadless])
			{
				_internals->device = [device retain];
				break;
			}
		}

		RN_ASSERT(_internals->device, "Needs a valid device");

		_internals->commandQueue = [_internals->device newCommandQueue];
		_internals->stateCoordinator.SetDevice(_internals->device);

		[devices release];

		// Texture format look ups
		_textureFormatLookup = new Dictionary();

#define TextureFormat(name, metal) \
		_textureFormatLookup->SetObjectForKey(Number::WithUint32(metal), RNCSTR(#name))

		TextureFormat(RGBA8888, MTLPixelFormatRGBA8Unorm);
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

#if RN_PLATFORM_MAC_OS
		if([_internals->device isDepth24Stencil8PixelFormatSupported])
		{
			TextureFormat(Depth24Stencil8, MTLPixelFormatDepth24Unorm_Stencil8);
		}
#endif

		TextureFormat(Depth32FStencil8, MTLPixelFormatDepth32Float_Stencil8);

#undef TextureFormat*/
	}

	D3D12Renderer::~D3D12Renderer()
	{
/*		[_internals->commandQueue release];
		[_internals->device release];

		_mipMapTextures->Release();
		_textureFormatLookup->Release();*/
	}


	Window *D3D12Renderer::CreateAWindow(const Vector2 &size, Screen *screen)
	{
		RN_ASSERT(!_mainWindow, "D3D12 renderer only supports one window");

		_mainWindow = new D3D12Window(size, screen, this);
		return _mainWindow;
	}

	Window *D3D12Renderer::GetMainWindow()
	{
		return _mainWindow;
	}


	void D3D12Renderer::CreateMipMapForeTexture(D3D12Texture *texture)
	{
		_lock.Lock();
		_mipMapTextures->AddObject(texture);
		_lock.Unlock();
	}

	void D3D12Renderer::CreateMipMaps()
	{
		if(_mipMapTextures->GetCount() == 0)
			return;

/*		id<MTLCommandBuffer> commandBuffer = [_internals->commandQueue commandBuffer];

		_mipMapTextures->Enumerate<D3D12Texture>([&](D3D12Texture *texture, bool &stop) {

			id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
			[commandEncoder generateMipmapsForTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture()];
			[commandEncoder endEncoding];
		});

		[commandBuffer commit];
		[commandBuffer waitUntilCompleted];
		*/
		_mipMapTextures->RemoveAllObjects();
	}


	void D3D12Renderer::RenderIntoWindow(Window *window, Function &&function)
	{
/*		_internals->pass.window = static_cast<D3D12Window *>(window);
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
		}*/
	}
	void D3D12Renderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		// Set up
/*		_internals->renderPass.camera = camera;
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
		D3D12Drawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(drawable);
			drawable = drawable->_next;
		}

		[_internals->renderPass.renderCommand endEncoding];
		[_internals->renderPass.renderCommand release];
		_internals->renderPass.renderCommand = nil;

		_internals->renderPass.activeState = nullptr;*/
	}

/*	MTLResourceOptions D3D12ResourceOptionsFromOptions(GPUResource::UsageOptions options)
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
	}*/

	GPUBuffer *D3D12Renderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions options)
	{
/*		MTLResourceOptions resourceOptions = D3D12ResourceOptionsFromOptions(options);
		id<MTLBuffer> buffer = [_internals->device newBufferWithLength:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new D3D12GPUBuffer(buffer));*/

		return new D3D12GPUBuffer(malloc(length), length);
	}
	GPUBuffer *D3D12Renderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options)
	{
/*		MTLResourceOptions resourceOptions = D3D12ResourceOptionsFromOptions(options);
		id<MTLBuffer> buffer = [_internals->device newBufferWithBytes:bytes length:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new D3D12GPUBuffer(buffer));*/

		return new D3D12GPUBuffer(malloc(length), length);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options)
	{
		return new D3D12ShaderLibrary(file);
	}
	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options)
	{
/*		MTLCompileOptions *metalOptions = [[MTLCompileOptions alloc] init];

		if(options)
		{
			const Dictionary *defines = options->GetDefines();
			if(defines)
			{
				NSMutableDictionary *metalDefines = [[NSMutableDictionary alloc] init];

				defines->Enumerate<Object, Object>([&](Object *value, const Object *key, bool &stop) {

					if(key->IsKindOfClass(String::GetMetaClass()))
					{
						NSString *keyString = [[NSString alloc] initWithUTF8String:static_cast<const String *>(key)->GetUTF8String()];

						if(value->IsKindOfClass(Number::GetMetaClass()))
						{
							NSNumber *valueNumber = [[NSNumber alloc] initWithLongLong:static_cast<Number *>(value)->GetInt64Value()];
							[metalDefines setObject:valueNumber forKey:keyString];
							[valueNumber release];
						}
						else if(value->IsKindOfClass(String::GetMetaClass()))
						{
							NSString *valueString = [[NSString alloc] initWithUTF8String:static_cast<const String *>(value)->GetUTF8String()];
							[metalDefines setObject:valueString forKey:keyString];
							[valueString release];
						}

						[keyString release];
					}

				});

				[metalOptions setPreprocessorMacros:metalDefines];
				[metalDefines release];
			}
		}

		NSError *error = nil;
		id<MTLLibrary> library = [_internals->device newLibraryWithSource:[NSString stringWithUTF8String:source->GetUTF8String()] options:metalOptions error:&error];


		[metalOptions release];

		if(!library)
			throw ShaderCompilationException([[error localizedDescription] UTF8String]);
			*/

		D3D12ShaderLibrary *lib = new D3D12ShaderLibrary(nullptr);
		return lib;
	}

	ShaderProgram *D3D12Renderer::GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup)
	{
		return nullptr;
	}

	bool D3D12Renderer::SupportsTextureFormat(const String *format) const
	{
		return (_textureFormatLookup->GetObjectForKey(format) != nullptr);
	}
	bool D3D12Renderer::SupportsDrawMode(DrawMode mode) const
	{
		return true;
	}

	// https://developer.apple.com/library/ios/documentation/D3D12/Reference/D3D12ShadingLanguageGuide/D3D12ShadingLanguageGuide.pdf
	size_t D3D12Renderer::GetAlignmentForType(PrimitiveType type) const
	{
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
				return 4;

			case PrimitiveType::Vector2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;
		}
	}

	size_t D3D12Renderer::GetSizeForType(PrimitiveType type) const
	{
		switch(type)
		{
			case PrimitiveType::Uint8:
			case PrimitiveType::Int8:
				return 1;

			case PrimitiveType::Uint16:
			case PrimitiveType::Int16:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
				return 4;

			case PrimitiveType::Vector2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			case PrimitiveType::Matrix:
				return 64;
		}
	}

	const String *D3D12Renderer::GetTextureFormatName(const Texture::Format format) const
	{
/*#define TextureFormatX(name) \
		case Texture::Format::name: \
			return RNCSTR(#name) \

		switch(format)
		{
			TextureFormatX(RGBA8888);
			TextureFormatX(RGB10A2);
			TextureFormatX(R8);
			TextureFormatX(RG88);
			TextureFormatX(R16F);
			TextureFormatX(RG16F);
			TextureFormatX(RGBA16F);
			TextureFormatX(R32F);
			TextureFormatX(RG32F);
			TextureFormatX(RGBA32F);
			TextureFormatX(Depth24I);
			TextureFormatX(Depth32F);
			TextureFormatX(Stencil8);
			TextureFormatX(Depth24Stencil8);
			TextureFormatX(Depth32FStencil8);

			default:
				return nullptr;
		}

#undef TextureFormatX*/

		return nullptr;
	}

	Texture *D3D12Renderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
/*		String *formatName = const_cast<String *>(descriptor.GetFormat());
		if(!formatName)
			throw InvalidTextureFormatException("Texture Format is NULL!");

		Number *format = _textureFormatLookup->GetObjectForKey<Number>(formatName);
		if(!format)
			throw InvalidTextureFormatException(RNSTR("Unsupported texture format '" << format << "'"));

		MTLTextureDescriptor *metalDescriptor = [[MTLTextureDescriptor alloc] init];

		metalDescriptor.width = descriptor.width;
		metalDescriptor.height = descriptor.height;
		metalDescriptor.depth = descriptor.depth;
		metalDescriptor.resourceOptions = D3D12ResourceOptionsFromOptions(descriptor.usageOptions);
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

		return new D3D12Texture(this, &_internals->stateCoordinator, texture, descriptor);*/

		return new D3D12Texture(this, nullptr, nullptr, descriptor);
	}

	Drawable *D3D12Renderer::CreateDrawable()
	{
		D3D12Drawable *drawable = new D3D12Drawable();
		drawable->_pipelineState = nullptr;
		drawable->_next = nullptr;
		drawable->_prev = nullptr;

		return drawable;
	}

	void D3D12Renderer::FillUniformBuffer(D3D12UniformBuffer *uniformBuffer, D3D12Drawable *drawable)
	{
		GPUBuffer *gpuBuffer = uniformBuffer->Advance();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());

		const D3D12UniformBuffer::Member *member;

		// Matrices
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::ModelMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), drawable->modelMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::ModelViewMatrix)))
		{
			Matrix result = _internals->renderPass.viewMatrix * drawable->modelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::ModelViewProjectionMatrix)))
		{
			Matrix result = _internals->renderPass.projectionViewMatrix * drawable->modelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::ViewMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.viewMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::ViewProjectionMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.projectionViewMatrix.m, sizeof(Matrix));
		}

		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::InverseModelMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), drawable->inverseModelMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::InverseModelViewMatrix)))
		{
			Matrix result = _internals->renderPass.inverseViewMatrix * drawable->inverseModelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::InverseModelViewProjectionMatrix)))
		{
			Matrix result = _internals->renderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
			std::memcpy(buffer + member->GetOffset(), result.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::InverseViewMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.inverseViewMatrix.m, sizeof(Matrix));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::InverseViewProjectionMatrix)))
		{
			std::memcpy(buffer + member->GetOffset(), _internals->renderPass.inverseProjectionViewMatrix.m, sizeof(Matrix));
		}

		// Color
		Material *material = drawable->material;

		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::AmbientColor)))
		{
			std::memcpy(buffer + member->GetOffset(), &material->GetAmbientColor().r, sizeof(Color));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::DiffuseColor)))
		{
			std::memcpy(buffer + member->GetOffset(), &material->GetDiffuseColor().r, sizeof(Color));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::SpecularColor)))
		{
			std::memcpy(buffer + member->GetOffset(), &material->GetSpecularColor().r, sizeof(Color));
		}
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::EmissiveColor)))
		{
			std::memcpy(buffer + member->GetOffset(), &material->GetEmissiveColor().r, sizeof(Color));
		}

		// Misc
		if((member = uniformBuffer->GetMemberForFeature(D3D12UniformBuffer::Feature::DiscardThreshold)))
		{
			float temp = material->GetDiscardThreshold();
			std::memcpy(buffer + member->GetOffset(), &temp, sizeof(float));
		}

		gpuBuffer->Invalidate();
	}

	void D3D12Renderer::SubmitDrawable(Drawable *tdrawable)
	{
		D3D12Drawable *drawable = static_cast<D3D12Drawable *>(tdrawable);

		if(drawable->dirty)
		{
			_lock.Lock();
			const D3D12RenderingState *state = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);
			_lock.Unlock();

			drawable->UpdateRenderingState(this, state);
			drawable->dirty = false;
		}

		// Update uniforms
		{
			for(D3D12UniformBuffer *uniformBuffer : drawable->_vertexBuffers)
			{
				FillUniformBuffer(uniformBuffer, drawable);
			}

			for(D3D12UniformBuffer *uniformBuffer : drawable->_fragmentBuffers)
			{
				FillUniformBuffer(uniformBuffer, drawable);
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

	void D3D12Renderer::RenderDrawable(D3D12Drawable *drawable)
	{
/*		id<MTLRenderCommandEncoder> encoder = _internals->renderPass.renderCommand;

		if(_internals->renderPass.activeState != drawable->_pipelineState)
		{
			[encoder setRenderPipelineState: drawable->_pipelineState->state];

			_internals->renderPass.activeState = drawable->_pipelineState;
		}

		[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(drawable->material)];


		// Set Uniforms
		for(D3D12UniformBuffer *uniformBuffer : drawable->_vertexBuffers)
		{
			D3D12GPUBuffer *buffer = static_cast<D3D12GPUBuffer *>(uniformBuffer->GetActiveBuffer());
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
		}

		for(D3D12UniformBuffer *uniformBuffer : drawable->_fragmentBuffers)
		{
			D3D12GPUBuffer *buffer = static_cast<D3D12GPUBuffer *>(uniformBuffer->GetActiveBuffer());
			[encoder setFragmentBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
		}

		// Set textures
		const Array *textures = drawable->material->GetTextures();
		size_t count = textures->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			D3D12Texture *texture = textures->GetObjectAtIndex<D3D12Texture>(i);

			[encoder setFragmentSamplerState:(id<MTLSamplerState>)texture->__GetUnderlyingSampler() atIndex:i];
			[encoder setFragmentTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture() atIndex:i];
		}


		// Mesh
		D3D12GPUBuffer *buffer = static_cast<D3D12GPUBuffer *>(drawable->mesh->GetVertexBuffer());
		[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:0];

		D3D12GPUBuffer *indexBuffer = static_cast<D3D12GPUBuffer *>(drawable->mesh->GetIndicesBuffer());
		[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:drawable->mesh->GetIndicesCount() indexType:MTLIndexTypeUInt16 indexBuffer:(id<MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];*/
	}
}
