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
#include "../../../Source/Rendering/RNShader.h"

namespace RN
{
	RNDefineMeta(MetalRenderer, Renderer)

	MetalRenderer::MetalRenderer(MetalRendererDescriptor *descriptor, MetalDevice *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_mipMapTextures(new Set()),
		_defaultShaderLibrary(nullptr)
	{
		_internals->device = device->GetDevice();
		_internals->commandQueue = [_internals->device newCommandQueue];
		_internals->stateCoordinator.SetDevice(_internals->device);
	}

	MetalRenderer::~MetalRenderer()
	{
		[_internals->commandQueue release];
		[_internals->device release];

		_mipMapTextures->Release();
	}


	Window *MetalRenderer::CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor)
	{
		MetalWindow *window = new MetalWindow(size, screen, this, descriptor);
		if(!_mainWindow)
			_mainWindow = window->Retain();

		return window;
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

		//TODO: make async
		[commandBuffer waitUntilCompleted];

		_mipMapTextures->RemoveAllObjects();
	}


	void MetalRenderer::Render(Function &&function)
	{
		_internals->renderPasses.clear();
		_internals->swapChains.clear();
		_internals->currentRenderState = nullptr;
		_internals->currentRenderPassIndex = 0;

		CreateMipMaps();

		//Submit camera is called for each camera and creates lists of drawables per camera
		function();

		for(MetalSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->AcquireBackBuffer();
			swapChain->Prepare();
		}

		_internals->commandBuffer = [[_internals->commandQueue commandBuffer] retain];

		_internals->currentRenderPassIndex = 0;
		for(MetalRenderPass &renderPass : _internals->renderPasses)
		{
			//TODO: Currently the next camera into the same framebuffer will clear the whole framebuffer...
			//There does not appear to be a way to only clear part of the framebuffer...
			const Color &clearColor = renderPass.camera->GetRenderPass()->GetClearColor();
			MTLRenderPassDescriptor *descriptor = [[MTLRenderPassDescriptor alloc] init];
			MTLRenderPassColorAttachmentDescriptor *colorAttachment = [[descriptor colorAttachments] objectAtIndexedSubscript:0];
			[colorAttachment setTexture:renderPass.framebuffer->GetRenderTarget()];
			[colorAttachment setLoadAction:MTLLoadActionClear];
			[colorAttachment setStoreAction:MTLStoreActionStore];
			[colorAttachment setClearColor:MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a)];

			if(renderPass.framebuffer->GetDepthStencilTexture())
			{
				MTLRenderPassDepthAttachmentDescriptor *depthAttachment = [descriptor depthAttachment];
				id<MTLTexture> depthTexture = static_cast< id<MTLTexture> >(renderPass.framebuffer->GetDepthStencilTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture());
				[depthAttachment setTexture:depthTexture];
				[depthAttachment setLoadAction:MTLLoadActionClear];
				[depthAttachment setStoreAction:MTLStoreActionStore];
			}

			if(renderPass.framebuffer->GetDepthStencilTexture())
			{
				MTLRenderPassStencilAttachmentDescriptor *stencilAttachment = [descriptor stencilAttachment];
				id<MTLTexture> stencilTexture = static_cast< id<MTLTexture> >(renderPass.framebuffer->GetDepthStencilTexture()->Downcast<MetalTexture>()->__GetUnderlyingTexture());
				[stencilAttachment setTexture:stencilTexture];
				[stencilAttachment setLoadAction:MTLLoadActionDontCare];
				[stencilAttachment setStoreAction:MTLStoreActionDontCare];
			}

			_internals->commandEncoder = [[_internals->commandBuffer renderCommandEncoderWithDescriptor:descriptor] retain];
			[descriptor release];

			Rect cameraRect = renderPass.camera->GetRenderPass()->GetFrame();
			if(cameraRect.width < 0.5f || cameraRect.height < 0.5f)
			{
				Vector2 framebufferSize = renderPass.framebuffer->GetSize();
				cameraRect.x = 0.0f;
				cameraRect.y = 0.0f;
				cameraRect.width = framebufferSize.x;
				cameraRect.height = framebufferSize.y;
			}
			MTLViewport viewPort;
			viewPort.originX = cameraRect.x;
			viewPort.originY = cameraRect.y;
			viewPort.width = cameraRect.width;
			viewPort.height = cameraRect.height;
			viewPort.znear = 0.0f;
			viewPort.zfar = 1.0f;
			[_internals->commandEncoder setViewport:viewPort];

			for(MetalDrawable *drawable : renderPass.drawables)
			{
				RenderDrawable(drawable);
			}

			[_internals->commandEncoder endEncoding];
			[_internals->commandEncoder release];
			_internals->commandEncoder = nil;

			_internals->currentRenderPassIndex += 1;
		}

		for(MetalSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->Finalize();
			swapChain->PresentBackBuffer(_internals->commandBuffer);
		}
		[_internals->commandBuffer commit];
		[_internals->commandBuffer release];
		_internals->commandBuffer = nil;
	}
	void MetalRenderer::SubmitCamera(Camera *camera, Function &&function)
	{
		// Set up
		MetalRenderPass currentRenderPass;
		currentRenderPass.camera = camera;
		currentRenderPass.drawables.resize(0);
		currentRenderPass.framebuffer = nullptr;

		if(camera->GetRenderPass()->GetFramebuffer())
			currentRenderPass.framebuffer = camera->GetRenderPass()->GetFramebuffer()->Downcast<MetalFramebuffer>();

		MetalSwapChain *newSwapChain = nullptr;
		if(!currentRenderPass.framebuffer)
		{
			MetalWindow *window = _mainWindow;
			newSwapChain = _mainWindow->GetSwapChain();
			currentRenderPass.framebuffer = newSwapChain->GetFramebuffer();
		}
		else
		{
			newSwapChain = currentRenderPass.framebuffer->GetSwapChain();
		}

		if(newSwapChain)
		{
			bool notIncluded = true;
			for(MetalSwapChain *swapChain : _internals->swapChains)
			{
				if(swapChain == newSwapChain)
				{
					notIncluded = false;
					break;
				}
			}
			if(notIncluded)
			{
				_internals->swapChains.push_back(newSwapChain);
			}
		}

		currentRenderPass.viewMatrix = camera->GetViewMatrix();
		currentRenderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		currentRenderPass.projectionMatrix = camera->GetProjectionMatrix();
		currentRenderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		currentRenderPass.projectionViewMatrix = currentRenderPass.projectionMatrix * currentRenderPass.viewMatrix;
		_internals->currentRenderPassIndex = _internals->renderPasses.size();
		_internals->renderPasses.push_back(currentRenderPass);

		// Create drawables
		function();
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

	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithFile(const String *file)
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

		return CreateShaderLibraryWithSource(source->Autorelease());
	}
	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithSource(const String *source)
	{
		MetalShaderLibrary *lib = new MetalShaderLibrary(_internals->device, source, &_internals->stateCoordinator);
		return lib;
	}

	Shader *MetalRenderer::GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint hint)
	{
		LockGuard<Lockable> lock(_lock);

		if(!_defaultShaderLibrary)
		{
			_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneMetal:/Shaders.json"));
		}

		Shader *shader = nullptr;
		if(type == Shader::Type::Vertex)
		{
/*			if(hint == Shader::UsageHint::Depth)
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("depth_vertex"), options);
			}
			else
			{*/
				const String *skyDefine = options? options->GetDefines()->GetObjectForKey<const String>(RNCSTR("RN_SKY")) : nullptr;
				if(skyDefine && !skyDefine->IsEqual(RNCSTR("0")))	//Use a different shader for the sky
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("sky_vertex"), options);
				}
				else
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("gouraud_vertex"), options);
				}
//			}
		}
		else if(type == Shader::Type::Fragment)
		{
/*			if(hint == Shader::UsageHint::Depth)
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("depth_fragment"), options);
			}
			else
			{*/
				const String *skyDefine = options? options->GetDefines()->GetObjectForKey<const String>(RNCSTR("RN_SKY")) : nullptr;
				if(skyDefine && !skyDefine->IsEqual(RNCSTR("0")))	//Use a different shader for the sky
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("sky_fragment"), options);
				}
				else
				{
					shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("gouraud_fragment"), options);
				}
//			}
		}

		return shader;
	}

	bool MetalRenderer::SupportsTextureFormat(const String *format) const
	{
		//TODO: Fix this
		return true;
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

	Texture *MetalRenderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		MTLTextureDescriptor *metalDescriptor = [[MTLTextureDescriptor alloc] init];

		metalDescriptor.width = descriptor.width;
		metalDescriptor.height = descriptor.height;
		metalDescriptor.depth = descriptor.depth;
		metalDescriptor.resourceOptions = MetalResourceOptionsFromOptions(descriptor.accessOptions);
		metalDescriptor.mipmapLevelCount = descriptor.mipMaps;
		metalDescriptor.pixelFormat = MetalTexture::PixelFormatForTextureFormat(descriptor.format);

		MTLTextureUsage usage = 0;

		if(descriptor.usageHint & Texture::UsageHint::ShaderRead)
			usage |= MTLTextureUsageShaderRead;
		if(descriptor.usageHint & Texture::UsageHint::ShaderWrite)
			usage |= MTLTextureUsageShaderWrite;
		if(descriptor.usageHint & Texture::UsageHint::RenderTarget)
		{
			usage |= MTLTextureUsageRenderTarget;
			metalDescriptor.storageMode = MTLStorageModePrivate;
		}

		metalDescriptor.usage = usage;


		switch(descriptor.type)
		{
			case Texture::Type::Type1D:
				metalDescriptor.textureType = MTLTextureType1D;
				break;
			case Texture::Type::Type1DArray:
				metalDescriptor.textureType = MTLTextureType1DArray;
				break;
			case Texture::Type::Type2D:
				metalDescriptor.textureType = MTLTextureType2D;
				break;
			case Texture::Type::Type2DArray:
				metalDescriptor.textureType = MTLTextureType2DArray;
				break;
			case Texture::Type::TypeCube:
				metalDescriptor.textureType = MTLTextureTypeCube;
				break;
			case Texture::Type::TypeCubeArray:
				metalDescriptor.textureType = MTLTextureTypeCubeArray;
				break;
			case Texture::Type::Type3D:
				metalDescriptor.textureType = MTLTextureType3D;
				break;
		}

		id<MTLTexture> texture = [_internals->device newTextureWithDescriptor:metalDescriptor];
		[metalDescriptor release];

		return new MetalTexture(this, texture, descriptor);
	}

	Framebuffer *MetalRenderer::CreateFramebuffer(const Vector2 &size)
	{
		return new MetalFramebuffer(size);
	}

	Drawable *MetalRenderer::CreateDrawable()
	{
		MetalDrawable *drawable = new MetalDrawable();
		return drawable;
	}

	void MetalRenderer::DeleteDrawable(Drawable *drawable)
	{
		delete drawable;
	}

	void MetalRenderer::FillUniformBuffer(MetalUniformBuffer *uniformBuffer, MetalDrawable *drawable, Shader *shader)
	{
		GPUBuffer *gpuBuffer = uniformBuffer->Advance();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());
		Material *material = drawable->material;

		const MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		shader->GetSignature()->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
			switch(descriptor->GetIdentifier())
			{
				//TODO: time, projection and inverseProjection are currently missing!

				case Shader::UniformDescriptor::Identifier::ModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->modelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewMatrix:
				{
					Matrix result = renderPass.viewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrix:
				{
					Matrix result = renderPass.projectionViewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.viewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.projectionViewMatrix.m, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::ProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.projectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->inverseModelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrix:
				{
					Matrix result = renderPass.inverseViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrix:
				{
					Matrix result = renderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseProjectionViewMatrix.m, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::InverseProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), renderPass.inverseProjectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AmbientColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &material->GetAmbientColor().r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DiffuseColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &material->GetDiffuseColor().r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpecularColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &material->GetSpecularColor().r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::EmissiveColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &material->GetEmissiveColor().r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::TextureTileFactor:
				{
					float temp = material->GetTextureTileFactor();
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DiscardThreshold:
				{
					float temp = material->GetDiscardThreshold();
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}
				
				//TODO: Use an extra reusable buffer for lights.
				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					uint32 lightCount = renderPass.directionalLights.size();
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, 4);
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					uint32 lightCount = renderPass.directionalLights.size();
					if(lightCount > 0)
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalLights[0], lightCount * (12 + 16));
					break;
				}

				case Shader::UniformDescriptor::Identifier::Custom:
				{
					//TODO: Implement custom shader variables!
					break;
				}

				default:
					break;
			}
		});

		gpuBuffer->Invalidate();
	}

	void MetalRenderer::SubmitLight(const Light *light)
	{
		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		if(light->GetType() == Light::Type::DirectionalLight)
		{
			if(renderPass.directionalLights.size() <= 5)
				renderPass.directionalLights.push_back(MetalDirectionalLight{light->GetForward(), light->GetColor()});
		}
		else if(light->GetType() == Light::Type::PointLight)
		{
			renderPass.pointLights.push_back(MetalPointLight{light->GetPosition(), light->GetColor(), light->GetRange()});
		}
		else if(light->GetType() == Light::Type::SpotLight)
		{
			renderPass.spotLights.push_back(MetalSpotLight{light->GetPosition(), light->GetForward(), light->GetColor(), light->GetRange(), light->GetAngleCos()});
		}
	}

	void MetalRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		MetalDrawable *drawable = static_cast<MetalDrawable *>(tdrawable);
		drawable->AddCameraSepecificsIfNeeded(_internals->currentRenderPassIndex);

		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		if(drawable->dirty)
		{
			_lock.Lock();
			const MetalRenderingState *state = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);
			_lock.Unlock();

			drawable->UpdateRenderingState(_internals->currentRenderPassIndex, this, state);
			drawable->dirty = false;
		}

		_lock.Lock();
		renderPass.drawables.push_back(drawable);
		_lock.Unlock();
	}

	void MetalRenderer::RenderDrawable(MetalDrawable *drawable)
	{
		// Update uniforms
		{
			//TODO: support multiple uniform buffer
			if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer)
				FillUniformBuffer(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer, drawable, drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState->vertexShader);
			if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer)
				FillUniformBuffer(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer, drawable, drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState->fragmentShader);
		}

		id<MTLRenderCommandEncoder> encoder = _internals->commandEncoder;

		if(_internals->currentRenderState != drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState)
		{
			[encoder setRenderPipelineState: drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState->state];

			_internals->currentRenderState = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState;
		}

		[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(drawable->material)];
		[encoder setCullMode:static_cast<MTLCullMode>(drawable->material->GetCullMode())];

		
		// Set Uniforms
		//TODO: support multiple uniform buffer
		size_t bufferIndex = 0;
		if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer)
		{
			MetalUniformBuffer *uniformBuffer = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer;
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBuffer->GetActiveBuffer());
			[encoder setVertexBuffer:(id <MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
		}

		bufferIndex = 0;
		if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer)
		{
			MetalUniformBuffer *uniformBuffer = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer;
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBuffer->GetActiveBuffer());
			[encoder setFragmentBuffer:(id <MTLBuffer>)buffer->_buffer offset:0 atIndex:uniformBuffer->GetIndex()];
		}

		// Set textures
		const Array *textures = drawable->material->GetTextures();
		size_t count = textures->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			MetalTexture *texture = textures->GetObjectAtIndex<MetalTexture>(i);
			[encoder setFragmentTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture() atIndex:i];
		}

		//Set samplers
		count = 0;
		for(void *sampler : drawable->material->GetVertexShader()->Downcast<MetalShader>()->_samplers)
		{
			id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
			[encoder setVertexSamplerState:samplerState atIndex:count++];
		}
		count = 0;
		for(void *sampler : drawable->material->GetFragmentShader()->Downcast<MetalShader>()->_samplers)
		{
			id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
			[encoder setFragmentSamplerState:samplerState atIndex:count++];
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
