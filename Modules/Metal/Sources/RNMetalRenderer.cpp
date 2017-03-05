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
#include "RNMetalDevice.h"
#include "RNMetalRendererDescriptor.h"
#include "RNMetalFramebuffer.h"

namespace RN
{
	RNDefineMeta(MetalRenderer, Renderer)

	MetalRenderer::MetalRenderer(MetalRendererDescriptor *descriptor, MetalDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_mipMapTextures(new Set()),
		_defaultShaders(new Dictionary())
	{
		_internals->device = device->GetDevice();
		_internals->commandQueue = [_internals->device newCommandQueue];
		_internals->stateCoordinator.SetDevice(_internals->device);

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
		TextureFormat(Depth32F, MTLPixelFormatDepth32Float);
		TextureFormat(Stencil8, MTLPixelFormatStencil8);

#if RN_PLATFORM_MAC_OS
		if([_internals->device isDepth24Stencil8PixelFormatSupported])
		{
			TextureFormat(Depth24I, MTLPixelFormatDepth24Unorm_Stencil8);
		}
#endif

		TextureFormat(Depth32FStencil8, MTLPixelFormatDepth32Float_Stencil8);

#undef TextureFormat
	}

	MetalRenderer::~MetalRenderer()
	{
		[_internals->commandQueue release];
		[_internals->device release];

		_mipMapTextures->Release();
		_textureFormatLookup->Release();
		_defaultShaders->Release();
	}


	Window *MetalRenderer::CreateAWindow(const Vector2 &size, Screen *screen)
	{
		RN_ASSERT(!_mainWindow, "Metal renderer only supports one window");

		_mainWindow = new MetalWindow(size, screen, this);
		return _mainWindow;
	}

	Window *MetalRenderer::GetMainWindow()
	{
		return _mainWindow;
	}


	void MetalRenderer::CreateMipMapForTexture(MetalTexture *texture)
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

		MetalDrawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(drawable);
			drawable = drawable->_next;
		}

		// Clean up
		[_internals->renderPass.renderCommand endEncoding];
		[_internals->renderPass.renderCommand release];
		_internals->renderPass.renderCommand = nil;

		_internals->renderPass.activeState = nullptr;
	}

	MTLResourceOptions MetalResourceOptionsFromOptions(GPUResource::AccessOptions options)
	{
		switch(options)
		{
			case GPUResource::AccessOptions::ReadWrite:
				return MTLResourceCPUCacheModeDefaultCache | MTLResourceStorageModeManaged;
			case GPUResource::AccessOptions::WriteOnly:
				return MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeManaged;
			case GPUResource::AccessOptions::Private:
				return  MTLResourceStorageModePrivate;
		}
	}

	GPUBuffer *MetalRenderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		MTLResourceOptions resourceOptions = MetalResourceOptionsFromOptions(accessOptions);
		id<MTLBuffer> buffer = [_internals->device newBufferWithLength:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new MetalGPUBuffer(buffer));
	}
	GPUBuffer *MetalRenderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		MTLResourceOptions resourceOptions = MetalResourceOptionsFromOptions(accessOptions);
		id<MTLBuffer> buffer = [_internals->device newBufferWithBytes:bytes length:length options:resourceOptions];
		if(!buffer)
			return nullptr;

		return (new MetalGPUBuffer(buffer));
	}

	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options)
	{
		Data *data = Data::WithContentsOfFile(file);

		Array *mainArray = JSONSerialization::ObjectFromData<Array>(data, 0);
		String *source = new String();

		mainArray->Enumerate<Dictionary>([&](Dictionary *libraryDictionary, size_t index, bool &stop) {

			String *file = libraryDictionary->GetObjectForKey<String>(RNCSTR("file~metal"));
			if(!file)
				file = libraryDictionary->GetObjectForKey<String>(RNCSTR("file"));
			if(!file)
				return;

			String *content = String::WithContentsOfFile(file, Encoding::UTF8);

			source->Append(content);
			source->Append("\n");

		});

		return CreateShaderLibraryWithSource(source->Autorelease(), options);
	}
	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options)
	{
		MTLCompileOptions *metalOptions = [[MTLCompileOptions alloc] init];

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

		MetalShaderLibrary *lib = new MetalShaderLibrary(library);
		return lib;
	}

	ShaderProgram *MetalRenderer::GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup)
	{
		Dictionary *defines = new Dictionary();

		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Normals))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_NORMALS"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Tangents))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_TANGENTS"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Color0))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_COLOR"));
		if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::UVCoords0))
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_UV0"));

		if(lookup->discard)
			defines->SetObjectForKey(Number::WithInt32(1), RNCSTR("RN_DISCARD"));

		ShaderCompileOptions *options = new ShaderCompileOptions();
		options->SetDefines(defines);

		ShaderLibrary *library;

		{
			LockGuard<Lockable> lock(_lock);
			library = _defaultShaders->GetObjectForKey<ShaderLibrary>(options);

			if(!library)
			{
				library = CreateShaderLibraryWithFile(RNCSTR(":RayneMetal:/Shaders.json"), options);
				_defaultShaders->SetObjectForKey(library, options);
			}
		}

		options->Release();
		defines->Release();

		Shader *vertex = library->GetShaderWithName(RNCSTR("gouraud_vertex"));
		Shader *fragment = library->GetShaderWithName(RNCSTR("gouraud_fragment"));

		ShaderProgram *program = new ShaderProgram(vertex, fragment);
		return program->Autorelease();
	}

	bool MetalRenderer::SupportsTextureFormat(const String *format) const
	{
		return (_textureFormatLookup->GetObjectForKey(format) != nullptr);
	}
	bool MetalRenderer::SupportsDrawMode(DrawMode mode) const
	{
		return true;
	}

	// https://developer.apple.com/library/ios/documentation/Metal/Reference/MetalShadingLanguageGuide/MetalShadingLanguageGuide.pdf
	size_t MetalRenderer::GetAlignmentForType(PrimitiveType type) const
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

	size_t MetalRenderer::GetSizeForType(PrimitiveType type) const
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

	const String *MetalRenderer::GetTextureFormatName(const Texture::Format format) const
	{
#define TextureFormatX(name) \
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

#undef TextureFormatX
	}

	Texture *MetalRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		String *formatName = const_cast<String *>(descriptor.GetFormat());
		if(!formatName)
			throw InvalidTextureFormatException("Texture Format is NULL!");

		Number *format = _textureFormatLookup->GetObjectForKey<Number>(formatName);
		if(!format)
			throw InvalidTextureFormatException(RNSTR("Unsupported texture format '" << formatName << "'"));

		MTLTextureDescriptor *metalDescriptor = [[MTLTextureDescriptor alloc] init];

		metalDescriptor.width = descriptor.width;
		metalDescriptor.height = descriptor.height;
		metalDescriptor.depth = descriptor.depth;
		metalDescriptor.resourceOptions = MetalResourceOptionsFromOptions(descriptor.accessOptions);
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

	Framebuffer *MetalRenderer::CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor)
	{
		return new MetalFramebuffer(size, descriptor);
	}

	Drawable *MetalRenderer::CreateDrawable()
	{
		MetalDrawable *drawable = new MetalDrawable();
		drawable->_pipelineState = nullptr;
		drawable->_next = nullptr;
		drawable->_prev = nullptr;

		return drawable;
	}

	void MetalRenderer::DeleteDrawable(Drawable *drawable)
	{
		delete drawable;
	}

	void MetalRenderer::FillUniformBuffer(MetalUniformBuffer *uniformBuffer, MetalDrawable *drawable)
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

		// Misc
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::DiscardThreshold)))
		{
			float temp = material->GetDiscardThreshold();
			std::memcpy(buffer + member->GetOffset(), &temp, sizeof(float));
		}
		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::TextureTileFactor)))
		{
			float temp = material->GetTextureTileFactor();
			std::memcpy(buffer + member->GetOffset(), &temp, sizeof(float));
		}

		gpuBuffer->Invalidate();
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
				if(uniformBuffer->IsActive())
					FillUniformBuffer(uniformBuffer, drawable);
			}

			for(MetalUniformBuffer *uniformBuffer : drawable->_fragmentBuffers)
			{
				if(uniformBuffer->IsActive())
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

	void MetalRenderer::RenderDrawable(MetalDrawable *drawable)
	{
		id<MTLRenderCommandEncoder> encoder = _internals->renderPass.renderCommand;

		if(_internals->renderPass.activeState != drawable->_pipelineState)
		{
			[encoder setRenderPipelineState: drawable->_pipelineState->state];

			_internals->renderPass.activeState = drawable->_pipelineState;
		}

		[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(drawable->material)];
		[encoder setCullMode:static_cast<MTLCullMode>(drawable->material->GetCullMode())];

		size_t bufferIndex = 0;

		// Set Uniforms
		const Array *vertexBuffers = drawable->material->GetVertexBuffers();

		for(MetalUniformBuffer *uniformBuffer : drawable->_vertexBuffers)
		{
			if(uniformBuffer->IsActive())
			{
				MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBuffer->GetActiveBuffer());
				[encoder setVertexBuffer:(id <MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
			}
			else
			{
				MetalGPUBuffer *buffer = vertexBuffers->GetObjectAtIndex<MetalGPUBuffer>(bufferIndex ++);
				[encoder setVertexBuffer:(id <MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
			}
		}

		const Array *fragmentBuffers = drawable->material->GetFragmentBuffers();
		bufferIndex = 0;

		for(MetalUniformBuffer *uniformBuffer : drawable->_fragmentBuffers)
		{
			if(uniformBuffer->IsActive())
			{
				MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBuffer->GetActiveBuffer());
				[encoder setFragmentBuffer:(id <MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
			}
			else
			{
				MetalGPUBuffer *buffer = fragmentBuffers->GetObjectAtIndex<MetalGPUBuffer>(bufferIndex ++);
				[encoder setFragmentBuffer:(id <MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
			}
		}

		// Set textures
		const Array *textures = drawable->material->GetTextures();
		size_t count = textures->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			MetalTexture *texture = textures->GetObjectAtIndex<MetalTexture>(i);

			[encoder setFragmentSamplerState:(id<MTLSamplerState>)texture->__GetUnderlyingSampler() atIndex:i];
			[encoder setFragmentTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture() atIndex:i];
		}


		// Mesh
		MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetVertexBuffer());
		[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:0];
		
		DrawMode drawMode = drawable->mesh->GetDrawMode();
		MTLPrimitiveType primitiveType;
		
		switch(drawMode)
		{
			case DrawMode::Point:
				primitiveType = MTLPrimitiveTypePoint;
				break;
			case DrawMode::Line:
				primitiveType = MTLPrimitiveTypeLine;
				break;
			case DrawMode::LineStrip:
				primitiveType = MTLPrimitiveTypeLineStrip;
				break;
			case DrawMode::Triangle:
				primitiveType = MTLPrimitiveTypeTriangle;
				break;
			case DrawMode::TriangleStrip:
				primitiveType = MTLPrimitiveTypeTriangleStrip;
				break;
		}
		
		if(drawable->mesh->GetIndicesCount() > 0)
		{
			MetalGPUBuffer *indexBuffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetIndicesBuffer());

			if(drawable->count == 1)
				[encoder drawIndexedPrimitives:primitiveType indexCount:drawable->mesh->GetIndicesCount() indexType:MTLIndexTypeUInt16 indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];
			else
				[encoder drawIndexedPrimitives:primitiveType indexCount:drawable->mesh->GetIndicesCount() indexType:MTLIndexTypeUInt16 indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0 instanceCount:drawable->count];
		}
		else
		{
			if(drawable->count == 1)
				[encoder drawPrimitives:primitiveType vertexStart:0 vertexCount:drawable->mesh->GetVerticesCount()];
			else
				[encoder drawPrimitives:primitiveType vertexStart:0 vertexCount:drawable->mesh->GetVerticesCount() instanceCount:drawable->count];
		}
	}
}
