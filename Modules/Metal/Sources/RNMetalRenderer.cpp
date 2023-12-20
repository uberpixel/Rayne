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
		_defaultShaderLibrary(nullptr),
		_currentMultiviewLayer(0),
		_currentMultiviewFallbackRenderPass(nullptr)
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
		_internals->currentRenderPassIndex = 0;

		CreateMipMaps();

		//Submit camera is called for each camera and creates lists of drawables per camera
		function();
		
		//Advance to next gpu buffer for uniforms and create new uniform buffer if pool is not big enough for any new objects
		_uniformBufferPool->Update(this); //This will also reset all reference offsets into the buffer

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
			
			_internals->currentRenderState = nullptr; //This is a property of the encoder and needs to be set to nullptr here to force setting it again.
			MTLRenderPassDescriptor *descriptor = renderPass.framebuffer->GetRenderPassDescriptor(renderPass.renderPass, renderPass.resolveFramebuffer, renderPass.multiviewLayer, 0);
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
				uint32 stepSize = 0;
				uint32 stepSizeIndex = 0;
				for(size_t i = 0; i < renderPass.drawables.size(); i+= stepSize)
				{
					stepSize = renderPass.instanceSteps[stepSizeIndex];
					stepSizeIndex += 1;
					
					uint32 counter = 0;
					const MetalDrawable::CameraSpecific &cameraSpecifics = renderPass.drawables[i]->_cameraSpecifics[_internals->currentRenderPassIndex];
					for(size_t n = 0; n < cameraSpecifics.vertexShaderUniformBuffers.size(); n++)
					{
						for(size_t instance = 0; instance < stepSize; instance++)
						{
							Shader::ArgumentBuffer *argument = cameraSpecifics.argumentBufferToUniformBufferMapping[counter];
							
							//TODO: Somehow find a better way to know if an argument buffer contains instance data or not
							//Assume that only storage buffers can contain per instance data
							if(instance > 0 && argument->GetType() != Shader::ArgumentBuffer::Type::StorageBuffer) break;
							
							MetalDrawable *drawable = renderPass.drawables[i + instance];
							Material::Properties mergedMaterialProperties = drawable->material->GetMergedProperties(renderPass.overrideMaterial);
							
							MetalUniformBufferReference *bufferReference = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexShaderUniformBuffers[n];
							UpdateUniformBufferReference(bufferReference, instance == 0);
							FillUniformBuffer(argument, bufferReference, drawable, mergedMaterialProperties);
						}
						counter += 1;
					}
					
					for(size_t n = 0; n < cameraSpecifics.fragmentShaderUniformBuffers.size(); n++)
					{
						for(size_t instance = 0; instance < stepSize; instance++)
						{
							Shader::ArgumentBuffer *argument = cameraSpecifics.argumentBufferToUniformBufferMapping[counter];
							
							//TODO: Somehow find a better way to know if an argument buffer contains instance data or not
							//Assume that only storage buffers can contain per instance data
							if(instance > 0 && argument->GetType() != Shader::ArgumentBuffer::Type::StorageBuffer) break;
							
							MetalDrawable *drawable = renderPass.drawables[i + instance];
							Material::Properties mergedMaterialProperties = drawable->material->GetMergedProperties(renderPass.overrideMaterial);
							
							MetalUniformBufferReference *bufferReference = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers[n];
							UpdateUniformBufferReference(bufferReference, instance == 0);
							FillUniformBuffer(argument, bufferReference, drawable, mergedMaterialProperties);
						}
						
						counter += 1;
					}
					
					RenderDrawable(renderPass.drawables[i], stepSize);
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
		
		//Flush all uniform buffers to make the GPU get the latest changes from CPU
		_uniformBufferPool->FlushAllBuffers();
		
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
				RenderDrawable(renderPass.drawables[0], 1);
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
				
				RN::Vector3 sourceTextureSize;
				
				if(sourceTexture)
				{
					sourceMTLTexture = static_cast< id<MTLTexture> >(sourceTexture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
					sourceTextureSize.x = sourceTexture->GetDescriptor().width;
					sourceTextureSize.y = sourceTexture->GetDescriptor().height;
					sourceTextureSize.z = sourceTexture->GetDescriptor().depth;
				}
				else
				{
					sourceMTLTexture = sourceFramebuffer->GetSwapChain()->GetMTLTexture();
					sourceTextureSize.x = sourceFramebuffer->GetSize().x;
					sourceTextureSize.y = sourceFramebuffer->GetSize().y;
					sourceTextureSize.z = 0;
				}
				
				if(destinationTexture)
				{
					destinationMTLTexture = static_cast< id<MTLTexture> >(destinationTexture->Downcast<MetalTexture>()->__GetUnderlyingTexture());
				}
				else
				{
					destinationMTLTexture = destinationFramebuffer->GetSwapChain()->GetMTLTexture();
				}
				
				MTLRenderPassDescriptor *descriptor = renderPass.framebuffer->GetRenderPassDescriptor(renderPass.renderPass, nullptr, 0, 0);
				id<MTLBlitCommandEncoder> commandEncoder = [[_internals->commandBuffer blitCommandEncoder] retain];
				[descriptor release];
				
				Rect targetRect = renderPass.renderPass->GetFrame();
				[commandEncoder copyFromTexture:sourceMTLTexture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(sourceTextureSize.x, sourceTextureSize.y, sourceTextureSize.z) toTexture:destinationMTLTexture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(targetRect.x, targetRect.y, 0)];
				
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
		const Array *multiviewCameras = camera->GetMultiviewCameras();
		if(multiviewCameras && multiviewCameras->GetCount() > 0)
		{
			//TODO: Multiview is not supported by metal, should use instanced with viewport selection instead (https://developer.apple.com/documentation/metal/mtlrenderpassdescriptor/rendering_to_multiple_texture_slices_in_a_draw_command?language=objc)
			/*if(multiviewCameras->GetCount() > 1 && GetVulkanDevice()->GetSupportsMultiview())
			{

			}
			else*/
			{
				//If multiview is not supported or there is only one multiview camera, render them individually into the correct framebuffer texture layers
				multiviewCameras->Enumerate<Camera>([&](Camera *multiviewCamera, size_t index, bool &stop){
					_currentMultiviewLayer = index;
					_currentMultiviewFallbackRenderPass = camera->GetRenderPass();

					SubmitCamera(multiviewCamera, std::move(function));

					_currentMultiviewLayer = 0;
					_currentMultiviewFallbackRenderPass = nullptr;
				});

				return;
			}
		}
		
		RenderPass *cameraRenderPass = _currentMultiviewFallbackRenderPass? _currentMultiviewFallbackRenderPass : camera->GetRenderPass();
		
		// Set up
		MetalRenderPass renderPass;
		renderPass.type = MetalRenderPass::Type::Default;
		renderPass.renderPass = cameraRenderPass;
		renderPass.previousRenderPass = nullptr;

		renderPass.drawables.resize(0);
		renderPass.multiviewLayer = _currentMultiviewLayer;

		renderPass.drawables.clear();
		renderPass.framebuffer = nullptr;

		renderPass.shaderHint = camera->GetShaderHint();
		renderPass.overrideMaterial = camera->GetMaterial();
		renderPass.resolveFramebuffer = nullptr;

		renderPass.camera = camera;
		
		renderPass.viewPosition = camera->GetWorldPosition();
		renderPass.viewMatrix = camera->GetViewMatrix();
		renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		renderPass.projectionMatrix = camera->GetProjectionMatrix();
		renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		renderPass.projectionViewMatrix = renderPass.projectionMatrix * renderPass.viewMatrix;
		renderPass.directionalShadowDepthTexture = nullptr;
		
		renderPass.cameraAmbientColor = camera->GetAmbientColor();
		
		renderPass.cameraClipDistance = Vector2(camera->GetClipNear(), camera->GetClipFar());
		renderPass.cameraFogDistance = Vector2(camera->GetFogNear(), camera->GetFogFar());
		renderPass.cameraFogColor0 = camera->GetFogColor0();
		renderPass.cameraFogColor1 = camera->GetFogColor1();

		Framebuffer *framebuffer = cameraRenderPass->GetFramebuffer();
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

		_internals->currentRenderState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;
		
		// Create drawables
		function();
		
		const Array *nextRenderPasses = cameraRenderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop) {
				SubmitRenderPass(nextPass, renderPass, std::forward<Function>(function));
			});
	}
	
	void MetalRenderer::SubmitRenderPass(RenderPass *renderPass, MetalRenderPass &previousRenderPass, Function &&function)
	{
		// Set up
		MetalRenderPass metalRenderPass;
		metalRenderPass.type = MetalRenderPass::Type::Default;
		metalRenderPass.renderPass = renderPass;
		metalRenderPass.previousRenderPass = previousRenderPass.renderPass;
		
		metalRenderPass.drawables.clear();
		metalRenderPass.framebuffer = nullptr;
		metalRenderPass.resolveFramebuffer = nullptr;
		
		_internals->currentRenderState = nullptr; //This is used when submitting drawables to make lists of drawables to instance and needs to be reset per render pass
		_internals->currentInstanceDrawable = nullptr;
		
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
		
		metalRenderPass.viewPosition = previousRenderPass.viewPosition;
		metalRenderPass.viewMatrix = previousRenderPass.viewMatrix;
		metalRenderPass.inverseViewMatrix = previousRenderPass.inverseViewMatrix;
		
		metalRenderPass.projectionMatrix = previousRenderPass.projectionMatrix;
		metalRenderPass.inverseProjectionMatrix = previousRenderPass.inverseProjectionMatrix;
		
		metalRenderPass.projectionViewMatrix = previousRenderPass.projectionViewMatrix;
		metalRenderPass.directionalShadowDepthTexture = nullptr;
		metalRenderPass.multiviewLayer = previousRenderPass.multiviewLayer;
		
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
			else
			{
				// Create drawables
				function();
			}
		}
		else
		{
			_internals->renderPasses[_internals->currentRenderPassIndex].resolveFramebuffer = metalRenderPass.framebuffer;
		}
		
		const Array *nextRenderPasses = renderPass->GetNextRenderPasses();
		nextRenderPasses->Enumerate<RenderPass>([&](RenderPass *nextPass, size_t index, bool &stop){
				SubmitRenderPass(nextPass, metalRenderPass, std::forward<Function>(function));
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

	void MetalRenderer::UpdateUniformBufferReference(MetalUniformBufferReference *reference, bool align)
	{
		LockGuard<Lockable> lock(_lock);
		return _uniformBufferPool->UpdateUniformBufferReference(reference, align);
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
			case PrimitiveType::Half:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
			case PrimitiveType::HalfVector2:
				return 4;

			case PrimitiveType::Vector2:
			case PrimitiveType::HalfVector3:
			case PrimitiveType::HalfVector4:
			case PrimitiveType::Matrix2x2:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix3x3:
			case PrimitiveType::Matrix4x4:
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
			case PrimitiveType::Half:
				return 2;

			case PrimitiveType::Uint32:
			case PrimitiveType::Int32:
			case PrimitiveType::Float:
			case PrimitiveType::HalfVector2:
				return 4;

			case PrimitiveType::Vector2:
			case PrimitiveType::HalfVector3:
			case PrimitiveType::HalfVector4:
				return 8;

			case PrimitiveType::Vector3:
			case PrimitiveType::Vector4:
			case PrimitiveType::Matrix2x2:
			case PrimitiveType::Quaternion:
			case PrimitiveType::Color:
				return 16;

			case PrimitiveType::Matrix3x3:
				return 48; //Stored as 3 x float4
				
			case PrimitiveType::Matrix4x4:
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

	void MetalRenderer::FillUniformBuffer(Shader::ArgumentBuffer *argument, MetalUniformBufferReference *uniformBufferReference, MetalDrawable *drawable, const Material::Properties &materialProperties)
	{
		GPUBuffer *gpuBuffer = uniformBufferReference->uniformBuffer->GetActiveBuffer();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer()) + uniformBufferReference->offset;

		const MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		argument->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
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
					
				case Shader::UniformDescriptor::Identifier::NormalMatrix:
				{
					Matrix normalMatrix = drawable->inverseModelMatrix.GetTransposed();
					std::memcpy(buffer + descriptor->GetOffset(), &normalMatrix.m[0], 12);
					std::memcpy(buffer + descriptor->GetOffset() + 16, &normalMatrix.m[4], 12);
					std::memcpy(buffer + descriptor->GetOffset() + 32, &normalMatrix.m[8], 12);
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
					Vector4 cameraPosition = Vector4(renderPass.viewPosition, 0.0f);
					std::memcpy(buffer + descriptor->GetOffset(), &cameraPosition.x, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::CameraClipDistance:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraClipDistance.x, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::CameraFogDistance:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogDistance.x, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::CameraAmbientColor:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraAmbientColor.r, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::CameraFogColor0:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogColor0.r, descriptor->GetSize());
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::CameraFogColor1:
				{
					std::memcpy(buffer + descriptor->GetOffset(), &renderPass.cameraFogColor1.r, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLightsCount:
				{
					uint32 lightCount = std::min(renderPass.directionalLights.size(), descriptor->GetElementCount());
					std::memcpy(buffer + descriptor->GetOffset(), &lightCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalLights:
				{
					size_t lightCount = std::min(renderPass.directionalLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.directionalLights[0], (16 + 16) * lightCount);
					}
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatricesCount:
				{
					//TODO: Limit matrixCount to descriptor->GetElementCount() of Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices
					uint32 matrixCount = renderPass.directionalShadowMatrices.size();
					std::memcpy(buffer + descriptor->GetOffset(), &matrixCount, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::DirectionalShadowMatrices:
				{
					size_t matrixCount = std::min(renderPass.directionalShadowMatrices.size(), descriptor->GetElementCount());
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
					size_t lightCount = std::min(renderPass.pointLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.pointLights[0], (12 + 4 + 16) * lightCount);
					}
					if(lightCount < descriptor->GetElementCount()) //TODO: Think about how max number of lights is filled up...
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 16) * lightCount, 0, (12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					break;
				}
				
				case Shader::UniformDescriptor::Identifier::SpotLights:
				{
					size_t lightCount = std::min(renderPass.spotLights.size(), descriptor->GetElementCount());
					if(lightCount > 0)
					{
						std::memcpy(buffer + descriptor->GetOffset(), &renderPass.spotLights[0], (12 + 4 + 12 + 4 + 16) * lightCount);
					}
					if(lightCount < descriptor->GetElementCount()) //TODO: Think about how max number of lights is filled up...
					{
						std::memset(buffer + descriptor->GetOffset() + (12 + 4 + 12 + 4 + 16) * lightCount, 0, (12 + 4 + 12 + 4 + 16) * (descriptor->GetElementCount() - lightCount));
					}
					
					break;
				}
					
				case Shader::UniformDescriptor::Identifier::BoneMatrices:
				{
					if(drawable->skeleton)
					{
						//TODO: Don't hardcode limit here
						size_t matrixCount = std::min(drawable->skeleton->_matrices.size(), descriptor->GetElementCount());
						if(matrixCount > 0)
						{
							std::memcpy(buffer + descriptor->GetOffset(), &drawable->skeleton->_matrices[0].m[0], 64 * matrixCount);
						}
					}
					break;
				}

				//TODO: Support arrays!
				case Shader::UniformDescriptor::Identifier::Custom:
				{
					Object *object = materialProperties.GetCustomShaderUniform(descriptor->GetNameHash());
					if(object)
					{
						if(object->IsKindOfClass(Value::GetMetaClass()))
						{
							Value *value = object->Downcast<Value>();
							switch(value->GetValueType())
							{
								case TypeTranslator<Vector2>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector2))
									{
										Vector2 vector = value->GetValue<Vector2>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector3>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector3))
									{
										Vector3 vector = value->GetValue<Vector3>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Vector4>::value:
								{
									if(descriptor->GetSize() == sizeof(Vector4))
									{
										Vector4 vector = value->GetValue<Vector4>();
										std::memcpy(buffer + descriptor->GetOffset(), &vector.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Matrix>::value:
								{
									if(descriptor->GetSize() == sizeof(Matrix))
									{
										Matrix matrix = value->GetValue<Matrix>();
										std::memcpy(buffer + descriptor->GetOffset(), &matrix.m[0], descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Quaternion>::value:
								{
									if(descriptor->GetSize() == sizeof(Quaternion))
									{
										Quaternion quaternion = value->GetValue<Quaternion>();
										std::memcpy(buffer + descriptor->GetOffset(), &quaternion.x, descriptor->GetSize());
									}
									break;
								}
								case TypeTranslator<Color>::value:
								{
									if(descriptor->GetSize() == sizeof(Color))
									{
										Color color = value->GetValue<Color>();
										std::memcpy(buffer + descriptor->GetOffset(), &color.r, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
						else
						{
							Number *number = object->Downcast<Number>();
							switch(number->GetType())
							{
								case Number::Type::Int8:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int8 value = number->GetInt8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int16:
								{
									if(descriptor->GetSize() == sizeof(int8))
									{
										int16 value = number->GetInt16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Int32:
								{
									if(descriptor->GetSize() == sizeof(int32))
									{
										int32 value = number->GetInt32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint8:
								{
									if(descriptor->GetSize() == sizeof(uint8))
									{
										uint8 value = number->GetUint8Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint16:
								{
									if(descriptor->GetSize() == sizeof(uint16))
									{
										uint16 value = number->GetUint16Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Uint32:
								{
									if(descriptor->GetSize() == sizeof(uint32))
									{
										uint32 value = number->GetUint32Value();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Float32:
								{
									if(descriptor->GetSize() == sizeof(float))
									{
										float value = number->GetFloatValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								case Number::Type::Boolean:
								{
									if(descriptor->GetSize() == sizeof(bool))
									{
										bool value = number->GetBoolValue();
										std::memcpy(buffer + descriptor->GetOffset(), &value, descriptor->GetSize());
									}
									break;
								}
								default:
									break;
							}
						}
					}
					break;
				}

				default:
					break;
			}
		});
	}

	void MetalRenderer::SubmitLight(const Light *light)
	{
		//TODO: Limit number of lights somehow!? Currently it is just limited by what the shader supports
		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		if(light->GetType() == Light::Type::DirectionalLight)
		{
			renderPass.directionalLights.push_back(MetalDirectionalLight{light->GetForward(), 0.0f, light->GetFinalColor()});

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
			renderPass.pointLights.push_back(MetalPointLight{light->GetWorldPosition(), light->GetRange(), light->GetFinalColor()});
		}
		else if(light->GetType() == Light::Type::SpotLight)
		{
			renderPass.spotLights.push_back(MetalSpotLight{light->GetWorldPosition(), light->GetRange(), light->GetForward(), light->GetAngleCos(), light->GetFinalColor()});
		}
	}

	void MetalRenderer::SubmitDrawable(Drawable *tdrawable)
	{
		MetalDrawable *drawable = static_cast<MetalDrawable *>(tdrawable);
		drawable->AddCameraSepecificsIfNeeded(_internals->currentRenderPassIndex);

		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];

		if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].dirty || drawable->_cameraSpecifics[_internals->currentRenderPassIndex].camera != renderPass.camera)
		{
			_lock.Lock();
			const MetalRenderingState *state = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, renderPass.framebuffer, renderPass.shaderHint, renderPass.overrideMaterial);
			_lock.Unlock();

			drawable->UpdateRenderingState(_internals->currentRenderPassIndex, renderPass.camera, this, state); //This will also reserve memory in a uniform buffer.
		}
		
		//Vertex and fragment shaders need to explicitly be marked to support instancing in the shader library json
		bool canUseInstancing = drawable->material->GetVertexShader()->GetHasInstancing() && drawable->material->GetFragmentShader()->GetHasInstancing();
		
		//Check if uniform buffers are the same, the object can't be part of the same instanced draw call if it doesn't share the same buffers (because they are full for example)
		if(canUseInstancing && _internals->currentInstanceDrawable && drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexShaderUniformBuffers.size() == _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexShaderUniformBuffers.size() && drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers.size() == _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers.size())
		{
			canUseInstancing = true;
			for(int i = 0; i < drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexShaderUniformBuffers.size() && canUseInstancing; i++)
			{
				if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexShaderUniformBuffers[i]->uniformBuffer != _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].vertexShaderUniformBuffers[i]->uniformBuffer)
				{
					canUseInstancing = false;
				}
			}
			
			for(int i = 0; i < drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers.size() && canUseInstancing; i++)
			{
				if(drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers[i]->uniformBuffer != _internals->currentInstanceDrawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers[i]->uniformBuffer)
				{
					canUseInstancing = false;
				}
			}
			
			if(canUseInstancing)
			{
				//depth testing and polygon offset are setup as part of the draw call and objects can only be instanced correctly if these are the same
				Material::Properties previousMergedMaterialProperties = drawable->material->GetMergedProperties(renderPass.overrideMaterial);
				Material::Properties mergedMaterialProperties = drawable->material->GetMergedProperties(renderPass.overrideMaterial);
				
				if(mergedMaterialProperties.depthMode != previousMergedMaterialProperties.depthMode || mergedMaterialProperties.depthWriteEnabled != previousMergedMaterialProperties.depthWriteEnabled || mergedMaterialProperties.usePolygonOffset != previousMergedMaterialProperties.usePolygonOffset || (mergedMaterialProperties.usePolygonOffset && (mergedMaterialProperties.polygonOffsetUnits != previousMergedMaterialProperties.polygonOffsetUnits || mergedMaterialProperties.polygonOffsetFactor != previousMergedMaterialProperties.polygonOffsetFactor)))
				{
					canUseInstancing = false;
				}
			}
		}
		
		if(_internals->currentRenderState == drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState && drawable->mesh == _internals->currentInstanceDrawable->mesh && drawable->material->GetTextures()->IsEqual(_internals->currentInstanceDrawable->material->GetTextures()) && canUseInstancing)
		{
			renderPass.instanceSteps.back() += 1; //Increase counter if the rendering state is the same
		}
		else
		{
			_internals->currentRenderState = drawable->_cameraSpecifics[_internals->currentRenderPassIndex].pipelineState;
			_internals->currentInstanceDrawable = drawable;
			renderPass.instanceSteps.push_back(1); //Add new entry if the rendering state changed
		}

		_lock.Lock();
		renderPass.drawables.push_back(drawable);
		_lock.Unlock();
	}

	void MetalRenderer::RenderDrawable(MetalDrawable *drawable, uint32 instanceCount)
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
		}
		
		MetalRenderPass &renderPass = _internals->renderPasses[_internals->currentRenderPassIndex];
		Material::Properties mergedMaterialProperties = drawable->material->GetMergedProperties(renderPass.overrideMaterial);
		[encoder setDepthStencilState:_internals->stateCoordinator.GetDepthStencilStateForMaterial(mergedMaterialProperties, _internals->currentRenderState)];
		[encoder setCullMode:static_cast<MTLCullMode>(mergedMaterialProperties.cullMode)];
		if(mergedMaterialProperties.usePolygonOffset)
		{
			[encoder setDepthBias:mergedMaterialProperties.polygonOffsetUnits slopeScale:mergedMaterialProperties.polygonOffsetFactor clamp:FLT_MAX];
		}
		else
		{
			[encoder setDepthBias:0.0f slopeScale:0.0f clamp:FLT_MAX];
		}
		
		// Update uniform buffers and set them for rendering
		{
			const MetalDrawable::CameraSpecific &cameraSpecifics = drawable->_cameraSpecifics[_internals->currentRenderPassIndex];
			for(MetalUniformBufferReference *uniformBufferReference : cameraSpecifics.vertexShaderUniformBuffers)
			{
				MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBufferReference->uniformBuffer->GetActiveBuffer());
				[encoder setVertexBuffer:(id <MTLBuffer>)buffer->_buffer offset:uniformBufferReference->offset atIndex:uniformBufferReference->shaderResourceIndex];
			}
			
			for(MetalUniformBufferReference *uniformBufferReference : drawable->_cameraSpecifics[_internals->currentRenderPassIndex].fragmentShaderUniformBuffers)
			{
				MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(uniformBufferReference->uniformBuffer->GetActiveBuffer());
				[encoder setFragmentBuffer:(id <MTLBuffer>)buffer->_buffer offset:uniformBufferReference->offset atIndex:uniformBufferReference->shaderResourceIndex];
			}
		}

		// Set textures
		//TODO: Support vertex shader textures
		const Array *textures = drawable->material->GetTextures();
		metalFragmentShader->GetSignature()->GetTextures()->Enumerate<Shader::ArgumentTexture>([&](Shader::ArgumentTexture *argument, size_t index, bool &stop){
			if(argument->GetMaterialTextureIndex() == Shader::ArgumentTexture::IndexDirectionalShadowTexture)
			{
				if(renderPass.directionalShadowDepthTexture)
				{
					[encoder setFragmentTexture:(id<MTLTexture>)renderPass.directionalShadowDepthTexture->__GetUnderlyingTexture() atIndex:argument->GetIndex()];
				}
				else
				{
					[encoder setFragmentTexture:nil atIndex:argument->GetIndex()];
				}
			}
			else
			{
				uint8 materialTextureIndex = argument->GetMaterialTextureIndex();
				if(materialTextureIndex < textures->GetCount())
				{
					MetalTexture *texture = textures->GetObjectAtIndex<MetalTexture>(materialTextureIndex);
					[encoder setFragmentTexture:(id<MTLTexture>)texture->__GetUnderlyingTexture() atIndex:argument->GetIndex()];
				}
				else
				{
					//TODO: handle post processing texture better
					if(materialTextureIndex == textures->GetCount() && renderPass.previousRenderPass && renderPass.previousRenderPass->GetFramebuffer())
					{
						MetalSwapChain *swapChain = renderPass.previousRenderPass->GetFramebuffer()->Downcast<MetalFramebuffer>()->GetSwapChain();
						if(swapChain)
						{
							[encoder setFragmentTexture:swapChain->GetMTLTexture() atIndex:argument->GetIndex()];
						}
						else
						{
							MetalTexture *colorBuffer = renderPass.previousRenderPass->GetFramebuffer()->GetColorTexture()->Downcast<MetalTexture>();
							[encoder setFragmentTexture:(id<MTLTexture>)colorBuffer->__GetUnderlyingTexture() atIndex:argument->GetIndex()];
						}
					}
					else
					{
						[encoder setFragmentTexture:nil atIndex:argument->GetIndex()];
					}
				}
			}
		});

		//Set samplers
		size_t count = 0;
		for(void *sampler : metalVertexShader->_samplers)
		{
			id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
			[encoder setVertexSamplerState:samplerState atIndex:metalFragmentShader->_samplerToIndexMapping[count++]];
		}
		count = 0;
		for(void *sampler : metalFragmentShader->_samplers)
		{
			id<MTLSamplerState> samplerState = static_cast<id<MTLSamplerState>>(sampler);
			[encoder setFragmentSamplerState:samplerState atIndex:metalFragmentShader->_samplerToIndexMapping[count++]];
		}

		// Mesh
		MetalGPUBuffer *buffer = static_cast<MetalGPUBuffer *>(drawable->mesh->GetGPUVertexBuffer());
		
		if(_internals->currentRenderState->vertexPositionBufferShaderResourceIndex <= 30)
		{
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:0 atIndex:_internals->currentRenderState->vertexPositionBufferShaderResourceIndex];
		}
		
		if(_internals->currentRenderState->vertexBufferShaderResourceIndex <= 30)
		{
			[encoder setVertexBuffer:(id<MTLBuffer>)buffer->_buffer offset:drawable->mesh->GetVertexPositionsSeparatedSize() atIndex:_internals->currentRenderState->vertexBufferShaderResourceIndex];
		}
		
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

			if(instanceCount == 1)
			{
				[encoder drawIndexedPrimitives:primitiveType indexCount:drawable->mesh->GetIndicesCount() indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0];
			}
			else
			{
				[encoder drawIndexedPrimitives:primitiveType indexCount:drawable->mesh->GetIndicesCount() indexType:indexType indexBuffer:(id <MTLBuffer>)indexBuffer->_buffer indexBufferOffset:0 instanceCount:instanceCount];
			}
		}
		else
		{
			if(instanceCount == 1)
			{
				[encoder drawPrimitives:primitiveType vertexStart:0 vertexCount:drawable->mesh->GetVerticesCount()];
			}
			else
			{
				[encoder drawPrimitives:primitiveType vertexStart:0 vertexCount:drawable->mesh->GetVerticesCount() instanceCount:instanceCount];
			}
		}
	}
}
