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
		_mipMapTextures(new Set()),
		_mainWindow(nullptr),
		_defaultPostProcessingDrawable(nullptr),
		_ppConvertMaterial(nullptr),
		_defaultShaderLibrary(nullptr)
	{
		_internals->device = device->GetDevice();
		_internals->commandQueue = [_internals->device newCommandQueue];
		_internals->stateCoordinator.SetDevice(_internals->device);
		
		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneMetal:/Shaders.json"));
		_uniformBufferPool = new MetalUniformBufferPool();
	}

	MetalRenderer::~MetalRenderer()
	{
		[_internals->commandQueue release];
		[_internals->device release];

		SafeRelease(_mipMapTextures);
		SafeRelease(_defaultShaderLibrary);
		
		delete _uniformBufferPool;
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
	
	void MetalRenderer::SetMainWindow(Window *window)
	{
		_mainWindow = window;
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
		
		//Advance to next gpu buffer for uniforms and create new uniform buffer if pool is not big enough for any new objects
		_uniformBufferPool->Update(this);

		for(MetalSwapChain *swapChain : _internals->swapChains)
		{
			//TODO: do this the first time the swap chain is actually used
			swapChain->AcquireBackBuffer();
			swapChain->Prepare();
		}

		_internals->commandBuffer = [[_internals->commandQueue commandBuffer] retain];

		_internals->currentRenderPassIndex = 0;
		for(MetalRenderPass &renderPass : _internals->renderPasses)
		{
			if(renderPass.framebuffer->GetSwapChain() && !renderPass.framebuffer->GetSwapChain()->GetMTLTexture())
			{
				_internals->currentRenderPassIndex += 1;
				continue;
			}
			
			if(renderPass.type != MetalRenderPass::Type::Default && renderPass.type != MetalRenderPass::Type::Convert)
			{
				RenderAPIRenderPass(renderPass);
				_internals->currentRenderPassIndex += 1;
				continue;
			}
			
			MTLRenderPassDescriptor *descriptor = renderPass.framebuffer->GetRenderPassDescriptor(renderPass.renderPass, renderPass.resolveFramebuffer);
			_internals->commandEncoder = [[_internals->commandBuffer renderCommandEncoderWithDescriptor:descriptor] retain];
			[descriptor release];

			Rect cameraRect = renderPass.renderPass->GetFrame();
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
			
			if(renderPass.type == MetalRenderPass::Type::Convert)
			{
				RenderAPIRenderPass(renderPass);
			}
			else
			{
				for(MetalDrawable *drawable : renderPass.drawables)
				{
					RenderDrawable(drawable);
				}
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
		
		//Invalidate all uniform buffers to make the GPU get the latest changes from CPU
		_uniformBufferPool->InvalidateAllBuffers();
		
		[_internals->commandBuffer commit];
		
		for(MetalSwapChain *swapChain : _internals->swapChains)
		{
			swapChain->PostPresent(_internals->commandBuffer);
		}
		
		[_internals->commandBuffer release];
		_internals->commandBuffer = nil;
	}
	
	void MetalRenderer::RenderAPIRenderPass(const MetalRenderPass &renderPass)
	{
		switch(renderPass.type)
		{
			case MetalRenderPass::Type::Convert:
			{
				MetalFramebuffer *sourceFramebuffer = renderPass.previousRenderPass->GetFramebuffer()->Downcast<RN::MetalFramebuffer>();
				Texture *sourceTexture = sourceFramebuffer->GetColorTexture(0);
				
				renderPass.drawables[0]->material->RemoveAllTextures();
				renderPass.drawables[0]->material->AddTexture(sourceTexture);
				RenderDrawable(renderPass.drawables[0]);
				break;
			}
				
			case MetalRenderPass::Type::Blit:
			{
				//TODO: Handle multiple and not existing textures
				MetalFramebuffer *sourceFramebuffer = renderPass.previousRenderPass->GetFramebuffer()->Downcast<RN::MetalFramebuffer>();
				Texture *sourceTexture = sourceFramebuffer->GetColorTexture(0);
				MetalFramebuffer *destinationFramebuffer = renderPass.renderPass->GetFramebuffer()->Downcast<RN::MetalFramebuffer>();
				Texture *destinationTexture = destinationFramebuffer->GetColorTexture(0);
				
				id<MTLTexture> sourceMTLTexture = nullptr;
				id<MTLTexture> destinationMTLTexture = nullptr;
				
				if(sourceTexture)
				{
					sourceMTLTexture = static_cast< id<MTLTexture> >(sourceTexture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
				}
				else
				{
					sourceMTLTexture = sourceFramebuffer->GetSwapChain()->GetMTLTexture();
				}
				
				if(destinationTexture)
				{
					destinationMTLTexture = static_cast< id<MTLTexture> >(destinationTexture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
				}
				else
				{
					destinationMTLTexture = destinationFramebuffer->GetSwapChain()->GetMTLTexture();
				}
				
				MTLRenderPassDescriptor *descriptor = renderPass.framebuffer->GetRenderPassDescriptor(renderPass.renderPass, nullptr);
				id<MTLBlitCommandEncoder> commandEncoder = [[_internals->commandBuffer blitCommandEncoder] retain];
				[descriptor release];
				
				Rect targetRect = renderPass.renderPass->GetFrame();
				[commandEncoder copyFromTexture:sourceMTLTexture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(sourceTexture->GetDescriptor().width, sourceTexture->GetDescriptor().height, sourceTexture->GetDescriptor().depth) toTexture:destinationMTLTexture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(targetRect.x, targetRect.y, 0)];
				
				[commandEncoder endEncoding];
				[commandEncoder release];
				
				break;
			}
			
			default:
				break;
		}
	}

	void MetalRenderer::SubmitCamera(Camera *camera, Function &&function)
	{
		// Set up
		MetalRenderPass renderPass;
		renderPass.type = MetalRenderPass::Type::Default;
		renderPass.renderPass = camera->GetRenderPass();
		renderPass.previousRenderPass = nullptr;

		renderPass.drawables.clear();
		renderPass.framebuffer = nullptr;

		renderPass.shaderHint = camera->GetShaderHint();
		renderPass.overrideMaterial = camera->GetMaterial();
		renderPass.resolveFramebuffer = nullptr;

		renderPass.viewPosition = camera->GetWorldPosition();
		renderPass.viewMatrix = camera->GetViewMatrix();
		renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		renderPass.projectionMatrix = camera->GetProjectionMatrix();
		renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		renderPass.projectionViewMatrix = renderPass.projectionMatrix * renderPass.viewMatrix;
		renderPass.directionalShadowDepthTexture = nullptr;
		
		renderPass.cameraAmbientColor = camera->GetAmbientColor();

		Framebuffer *framebuffer = camera->GetRenderPass()->GetFramebuffer();
		MetalSwapChain *newSwapChain = nullptr;
		newSwapChain = framebuffer->Downcast<MetalFramebuffer>()->GetSwapChain();
		renderPass.framebuffer = framebuffer->Downcast<MetalFramebuffer>();

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

		_internals->currentRenderPassIndex = _internals->renderPasses.size();
		_internals->renderPasses.push_back(renderPass);

		// Create drawables
		function();
		
		const Array *nextRenderPasses = renderPass.renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop) {
				SubmitRenderPass(nextPass, renderPass.renderPass);
			});
	}
	
	void MetalRenderer::SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass)
	{
		// Set up
		MetalRenderPass metalRenderPass;
		metalRenderPass.type = MetalRenderPass::Type::Default;
		metalRenderPass.renderPass = renderPass;
		metalRenderPass.previousRenderPass = previousRenderPass;
		
		metalRenderPass.drawables.clear();
		metalRenderPass.framebuffer = nullptr;
		metalRenderPass.resolveFramebuffer = nullptr;
		
		PostProcessingAPIStage *apiStage = renderPass->Downcast<PostProcessingAPIStage>();
		PostProcessingStage *ppStage = renderPass->Downcast<PostProcessingStage>();
		if(!apiStage)
		{
			metalRenderPass.type = MetalRenderPass::Type::Default;
			metalRenderPass.overrideMaterial = ppStage? ppStage->GetMaterial() : nullptr;
		}
		else
		{
			switch(apiStage->GetType())
			{
				case PostProcessingAPIStage::Type::ResolveMSAA:
				{
					metalRenderPass.type = MetalRenderPass::Type::ResolveMSAA;
					break;
				}
					
				case PostProcessingAPIStage::Type::Convert:
				{
					metalRenderPass.type = MetalRenderPass::Type::Convert;
					
					if(!_ppConvertMaterial)
					{
						_ppConvertMaterial = Material::WithShaders(_defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_vertex")), _defaultShaderLibrary->GetShaderWithName(RNCSTR("pp_blit_fragment")))->Retain();
					}
					metalRenderPass.overrideMaterial = _ppConvertMaterial;
					break;
				}
				
				case PostProcessingAPIStage::Type::Blit:
				{
					metalRenderPass.type = MetalRenderPass::Type::Blit;
					break;
				}
			}
		}
		
		metalRenderPass.shaderHint = Shader::UsageHint::Default;
		
		//TODO: Set matrices based on last camera renderpass?
		/*metalRenderPass.viewPosition = camera->GetWorldPosition();
		metalRenderPass.viewMatrix = camera->GetViewMatrix();
		metalRenderPass.inverseViewMatrix = camera->GetInverseViewMatrix();
		
		metalRenderPass.projectionMatrix = camera->GetProjectionMatrix();
		metalRenderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();
		
		metalRenderPass.projectionViewMatrix = renderPass.projectionMatrix * renderPass.viewMatrix;*/
		metalRenderPass.directionalShadowDepthTexture = nullptr;
		
		Framebuffer *framebuffer = renderPass->GetFramebuffer();
		MetalSwapChain *newSwapChain = nullptr;
		newSwapChain = framebuffer->Downcast<MetalFramebuffer>()->GetSwapChain();
		metalRenderPass.framebuffer = framebuffer->Downcast<MetalFramebuffer>();
		
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
		
		if(metalRenderPass.type != MetalRenderPass::Type::ResolveMSAA)
		{
			_internals->currentRenderPassIndex = _internals->renderPasses.size();
			_internals->renderPasses.push_back(metalRenderPass);
			
			if(ppStage || metalRenderPass.type == MetalRenderPass::Type::Convert)
			{
				//Submit fullscreen quad drawable
				if(!_defaultPostProcessingDrawable)
				{
					Mesh *planeMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f, 0.0f)), Vector3(0.0f), Vector2(1.0f, 1.0f));
					Material *planeMaterial = Material::WithShaders(GetDefaultShader(Shader::Type::Vertex, nullptr), GetDefaultShader(Shader::Type::Fragment, nullptr));
					
					_lock.Lock();
					_defaultPostProcessingDrawable = static_cast<MetalDrawable*>(CreateDrawable());
					_defaultPostProcessingDrawable->mesh = planeMesh->Retain();
					_defaultPostProcessingDrawable->material = planeMaterial->Retain();
					_lock.Unlock();
				}
				SubmitDrawable(_defaultPostProcessingDrawable);
			}
		}
		else
		{
			_internals->renderPasses[_internals->currentRenderPassIndex].resolveFramebuffer = metalRenderPass.framebuffer;
		}
		
		const Array *nextRenderPasses = renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop){
				SubmitRenderPass(nextPass, renderPass);
			});
	}

	//TODO: Move into an utility class
	MTLResourceOptions MetalRenderer::MetalResourceOptionsFromOptions(GPUResource::AccessOptions options)
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
	
	MetalUniformBufferReference *MetalRenderer::GetUniformBufferReference(size_t size, size_t index)
	{
		LockGuard<Lockable> lock(_lock);
		return _uniformBufferPool->GetUniformBufferReference(size, index);
	}

	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithFile(const String *file)
	{
		MetalShaderLibrary *lib = new MetalShaderLibrary(_internals->device, file, &_internals->stateCoordinator);
		return lib;
	}
	ShaderLibrary *MetalRenderer::CreateShaderLibraryWithSource(const String *source)
	{
		MetalShaderLibrary *lib = new MetalShaderLibrary(_internals->device, nullptr, &_internals->stateCoordinator);
		return lib;
	}

	Shader *MetalRenderer::GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint hint)
	{
		LockGuard<Lockable> lock(_lock);

		ShaderLibrary *shaderLibrary = _defaultShaderLibrary;

		Shader *shader = nullptr;
		if(type == Shader::Type::Vertex)
		{
			if(hint == Shader::UsageHint::Depth)
			{
				shader = shaderLibrary->GetShaderWithName(RNCSTR("depth_vertex"), options);
			}
			else
			{
				const String *skyDefine = options? options->GetDefines()->GetObjectForKey<const String>(RNCSTR("RN_SKY")) : nullptr;
				if(skyDefine && !skyDefine->IsEqual(RNCSTR("0")))	//Use a different shader for the sky
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("sky_vertex"), options);
				}
				else
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("gouraud_vertex"), options);
				}
			}
		}
		else if(type == Shader::Type::Fragment)
		{
			if(hint == Shader::UsageHint::Depth)
			{
				shader = shaderLibrary->GetShaderWithName(RNCSTR("depth_fragment"), options);
			}
			else
			{
				const String *skyDefine = options? options->GetDefines()->GetObjectForKey<const String>(RNCSTR("RN_SKY")) : nullptr;
				if(skyDefine && !skyDefine->IsEqual(RNCSTR("0")))	//Use a different shader for the sky
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("sky_fragment"), options);
				}
				else
				{
					shader = shaderLibrary->GetShaderWithName(RNCSTR("gouraud_fragment"), options);
				}
			}
		}

		return shader;
	}
	
	ShaderLibrary *MetalRenderer::GetDefaultShaderLibrary()
	{
		return _defaultShaderLibrary;
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
		MTLTextureDescriptor *metalDescriptor = MetalTexture::DescriptorForTextureDescriptor(descriptor);
		id<MTLTexture> texture = [_internals->device newTextureWithDescriptor:metalDescriptor];
		[metalDescriptor release];

		return new MetalTexture(this, texture, descriptor);
	}
	
	Texture *MetalRenderer::CreateTextureWithDescriptorAndIOSurface(const Texture::Descriptor &descriptor, IOSurfaceRef ioSurface)
	{
		MTLTextureDescriptor *metalDescriptor = MetalTexture::DescriptorForTextureDescriptor(descriptor, true);
		id<MTLTexture> texture = [_internals->device newTextureWithDescriptor:metalDescriptor iosurface:ioSurface plane:0];
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

	void MetalRenderer::FillUniformBuffer(MetalUniformBufferReference *uniformBufferReference, MetalDrawable *drawable, Shader *shader, const Material::Properties &materialProperties)
	{
		GPUBuffer *gpuBuffer = uniformBufferReference->uniformBuffer->GetActiveBuffer();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer()) + uniformBufferReference->offset;

		const MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		shader->GetSignature()->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
			switch(descriptor->GetIdentifier())
			{
				case Shader::UniformDescriptor::Identifier::Time:
				{
					float temp = static_cast<float>(Kernel::GetSharedInstance()->GetTotalTime());
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

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
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.ambientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DiffuseColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.diffuseColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::SpecularColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.specularColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::EmissiveColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.emissiveColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::TextureTileFactor:
				{
					float temp = materialProperties.textureTileFactor;
					std::memcpy(buffer + descriptor->GetOffset(), &temp, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::AlphaToCoverageClamp:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &materialProperties.alphaToCoverageClamp.x, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::CameraPosition:
				{
					RN::Vector3 cameraPosition = renderPass.viewPosition;
					std::memcpy(buffer + descriptor->GetOffset(), &cameraPosition.x, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::CameraAmbientColor:
				{
					RN::Color cameraAmbientColor = renderPass.cameraAmbientColor;
					std::memcpy(buffer + descriptor->GetOffset(), &cameraAmbientColor.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					int16 lightCount = renderPass.directionalLights.size();
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					size_t lightCount = renderPass.directionalLights.size();
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalLights[0], (12 + 16) * lightCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatricesCount:
				{
					int16 matrixCount = renderPass.directionalShadowMatrices.size();
					if(matrixCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &matrixCount, descriptor->GetSize());
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices:
				{
					size_t matrixCount = renderPass.directionalShadowMatrices.size();
					if(matrixCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalShadowMatrices[0].m[0], 64 * matrixCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowInfo:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalShadowInfo.x, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::PointLights:
				{
					size_t lightCount = renderPass.pointLights.size();
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.pointLights[0], (12 + 4 + 16) * lightCount);
					}
					if(lightCount < 8) //TODO: Think about how max number of lights is filled up...
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 16) * lightCount, 0, (12 + 4 + 16) * (8-lightCount));
					}
					break;
				}
				
				case Shader::UniformDescriptor::Identifier::SpotLights:
				{
					size_t lightCount = renderPass.spotLights.size();
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.spotLights[0], (12 + 4 + 12 + 4 + 16) * lightCount);
					}
					if(lightCount < 8) //TODO: Think about how max number of lights is filled up...
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 12 + 4 + 16) * lightCount, 0, (12 + 4 + 12 + 4 + 16) * (8-lightCount));
					}
					
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
	}

	void MetalRenderer::SubmitLight(const Light *light)
	{
		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		if(light->GetType() == Light::Type::DirectionalLight)
		{
			if(renderPass.directionalLights.size() < 5) //TODO: Don't hardcode light limit here
				renderPass.directionalLights.push_back(MetalDirectionalLight{light->GetForward(), light->GetColor()});

			//TODO: Allow more lights with shadows or prevent multiple light with shadows overwriting each other
			if(light->HasShadows())
			{
				renderPass.directionalShadowDepthTexture = light->GetShadowDepthTexture()->Downcast<MetalTexture>();
				renderPass.directionalShadowMatrices = light->GetShadowMatrices();
				renderPass.directionalShadowInfo = Vector2(1.0f / light->GetShadowParameters().resolution);
			}
		}
		else if(light->GetType() == Light::Type::PointLight)
		{
			if(renderPass.pointLights.size() < 8) //TODO: Don't hardcode light limit here
				renderPass.pointLights.push_back(MetalPointLight{light->GetWorldPosition(), light->GetRange(), light->GetColor()});
		}
		else if(light->GetType() == Light::Type::SpotLight)
		{
			if(renderPass.spotLights.size() < 8) //TODO: Don't hardcode light limit here
				renderPass.spotLights.push_back(MetalSpotLight{light->GetWorldPosition(), light->GetRange(), light->GetForward(), light->GetAngleCos(), light->GetColor()});
		}
	}

	void MetalRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		MetalDrawable *drawable = static_cast<MetalDrawable *>(tdrawable);
		drawable->AddCameraSepecificsIfNeeded(_internals->currentRenderPassIndex);

		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].dirty)
		{
			_lock.Lock();
			const MetalRenderingState *state = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, renderPass.framebuffer, renderPass.shaderHint, renderPass.overrideMaterial);
			_lock.Unlock();

			drawable->UpdateRenderingState(_internals->currentRenderPassIndex, this, state);
		}

		_lock.Lock();
		renderPass.drawables.push_back(drawable);
		_lock.Unlock();
	}

	void MetalRenderer::RenderDrawable(MetalDrawable *drawable)
	{
		id<MTLRenderCommandEncoder> encoder = _internals->commandEncoder;
		if(_internals->currentRenderState != drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState)
		{
			_internals->currentRenderState = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState;
			[encoder setRenderPipelineState: _internals->currentRenderState->state];
		}
		
		Shader *vertexShader = _internals->currentRenderState->vertexShader;
		Shader *fragmentShader = _internals->currentRenderState->fragmentShader;
		MetalShader *metalVertexShader = nullptr;
		MetalShader *metalFragmentShader = nullptr;
		if(vertexShader)
		{
			metalVertexShader = vertexShader->Downcast<MetalShader>();
		}
		if(fragmentShader)
		{
			metalFragmentShader = fragmentShader->Downcast<MetalShader>();
			if(metalFragmentShader->_samplers.size() == 0)
			{
				__unused int i = 0;
			}
		}
		
		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		Material::Properties mergedMaterialProperties = drawable->material->GetMergedProperties(renderPass.overrideMaterial);
		
		[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(mergedMaterialProperties, _internals->currentRenderState)];
		[encoder setCullMode:static_cast<MTLCullMode>(mergedMaterialProperties.cullMode)];
		
		if(mergedMaterialProperties.usePolygonOffset)
		{
			[encoder setDepthBias:mergedMaterialProperties.polygonOffsetUnits slopeScale:mergedMaterialProperties.polygonOffsetFactor clamp:FLT_MAX];
		}
		
		// Update uniforms
		{
			//TODO: support multiple uniform buffer
			if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer)
				FillUniformBuffer(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer, drawable, metalVertexShader, mergedMaterialProperties);
			if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer)
				FillUniformBuffer(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer, drawable, metalFragmentShader, mergedMaterialProperties);
		}

		// Set Uniforms
		//TODO: support multiple uniform buffer
		size_t bufferIndex = 0;
		if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer)
		{
			MetalUniformBufferReference *uniformBufferReference = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexBuffer;
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBufferReference->uniformBuffer->GetActiveBuffer());
			[encoder setVertexBuffer:(id <MTLBuffer>)buffer->_buffer offset:uniformBufferReference->offset atIndex:uniformBufferReference->shaderResourceIndex];
		}

		bufferIndex = 0;
		if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer)
		{
			MetalUniformBufferReference *uniformBufferReference = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentBuffer;
			MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBufferReference->uniformBuffer->GetActiveBuffer());
			[encoder setFragmentBuffer:(id <MTLBuffer>)buffer->_buffer offset:uniformBufferReference->offset atIndex:uniformBufferReference->shaderResourceIndex];
		}

		// Set textures
		const Array *textures = drawable->material->GetTextures();
		size_t count = metalFragmentShader->GetSignature()->GetTextureCount();

		for(size_t i = 0; i < count; i ++)
		{
			//TODO: handle shadow texture better
			if(i == count - 1 && _internals->currentRenderState->wantsShadowTexture)
			{
				if(renderPass.directionalShadowDepthTexture)
				{
					[encoder setFragmentTexture:(id<MTLTexture>)renderPass.directionalShadowDepthTexture->__GetUnderlyingTexture() atIndex:i];
				}
				else
				{
					[encoder setFragmentTexture:nil atIndex:i];
				}
				continue;
			}

			if(i < textures->GetCount())
			{
				MetalTexture *texture = textures->GetObjectAtIndex<MetalTexture>(i);
				[encoder setFragmentTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture() atIndex:i];
			}
			else
			{
				//TODO: handle post processing texture better
				if(i == textures->GetCount() && renderPass.previousRenderPass && renderPass.previousRenderPass->GetFramebuffer())
				{
					MetalTexture *colorBuffer = renderPass.previousRenderPass->GetFramebuffer()->GetColorTexture()->Downcast<MetalTexture>();
					[encoder setFragmentTexture:(id<MTLTexture>)colorBuffer->__GetUnderlyingTexture() atIndex:i];
				}
				else
				{
					[encoder setFragmentTexture:nil atIndex:i];
				}
			}
		}

		//Set samplers
		count = 0;
		for(void *sampler : metalVertexShader->_samplers)
		{
			id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
			[encoder setVertexSamplerState:samplerState atIndex:count++];
		}
		count = 0;
		for(void *sampler : metalFragmentShader->_samplers)
		{
			id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
			[encoder setFragmentSamplerState:samplerState atIndex:count++];
		}

		// Mesh
		MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetGPUVertexBuffer());
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
			MetalGPUBuffer *indexBuffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetGPUIndicesBuffer());
			MTLIndexType indexType = drawable->mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices)->GetType() == PrimitiveType::Uint16? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;

			if(drawable->count == 1)
				[encoder drawIndexedPrimitives:primitiveType indexCount:drawable->mesh->GetIndicesCount() indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];
			else
				[encoder drawIndexedPrimitives:primitiveType indexCount:drawable->mesh->GetIndicesCount() indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0 instanceCount:drawable->count];
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
