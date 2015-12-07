//
//  RND3D12Renderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Internals.h"
#include "RND3D12Renderer.h"
#include "RND3D12ShaderLibrary.h"
#include "RND3D12GPUBuffer.h"
#include "RND3D12Texture.h"
#include "RND3D12UniformBuffer.h"

namespace RN
{
	RNDefineMeta(D3D12Renderer, Renderer)

	D3D12Renderer::D3D12Renderer(const Dictionary *parameters) :
		_mainWindow(nullptr),
		_mipMapTextures(new Set()),
		_defaultShaders(new Dictionary())
	{
		_internals->device = nullptr;

#ifdef _DEBUG
		// Enable the D3D12 debug layer.
		{
			ID3D12Debug *debugController;
			if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
		}
#endif
		
		// Texture format look ups
		_textureFormatLookup = new Dictionary();

#define TextureFormat(name, d3d12) \
		_textureFormatLookup->SetObjectForKey(Number::WithUint32(d3d12), RNCSTR(#name))

		TextureFormat(RGBA8888, DXGI_FORMAT_R8G8B8A8_UNORM);
		TextureFormat(RGB10A2, DXGI_FORMAT_R10G10B10A2_UNORM);
		TextureFormat(R8, DXGI_FORMAT_R8_UNORM);
		TextureFormat(RG88, DXGI_FORMAT_R8G8_UNORM);
		TextureFormat(R16F, DXGI_FORMAT_R16_FLOAT);
		TextureFormat(RG16F, DXGI_FORMAT_R16G16_FLOAT);
		TextureFormat(RGBA16F, DXGI_FORMAT_R16G16B16A16_FLOAT);
		TextureFormat(R32F, DXGI_FORMAT_R32_FLOAT);
		TextureFormat(RG32F, DXGI_FORMAT_R32G32_FLOAT);
		TextureFormat(RGBA32F, DXGI_FORMAT_R32G32B32A32_FLOAT);
		TextureFormat(Depth24I, DXGI_FORMAT_D24_UNORM_S8_UINT);
		TextureFormat(Depth32F, DXGI_FORMAT_D32_FLOAT);
		TextureFormat(Depth32FStencil8, DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

#undef TextureFormat
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
		_internals->InitializeRenderingPipeline(_mainWindow);

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
		_internals->pass.window = static_cast<D3D12Window *>(window);

		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		_internals->commandAllocators[_internals->frameIndex]->Reset();

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		_internals->commandList->Reset(_internals->commandAllocators[_internals->frameIndex], NULL);

		/*if(_needsUpdateForWindowSizeChange)
		{
			CreateFramebuffers();
		}*/

		// Set necessary state.
		_internals->commandList->SetGraphicsRootSignature(_internals->rootSignature);

		ID3D12DescriptorHeap* ppHeaps[] = { _internals->cbvHeap };
		_internals->commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		_internals->commandList->SetGraphicsRootDescriptorTable(0, _internals->cbvHeap->GetGPUDescriptorHandleForHeapStart());

		// Draw cameras
		function();

		// Indicate that the back buffer will now be used to present.
		_internals->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_internals->renderTargets[_internals->frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		_internals->commandList->Close();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { _internals->commandList };
		_internals->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		_internals->swapChain->Present(0, 0);

		_internals->MoveToNextFrame();
	}

	void D3D12Renderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		_internals->commandList->RSSetViewports(1, &_internals->viewport);
		_internals->commandList->RSSetScissorRects(1, &_internals->scissorRect);

		// Indicate that the back buffer will be used as a render target.
		_internals->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_internals->renderTargets[_internals->frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_internals->rtvHeap->GetCPUDescriptorHandleForHeapStart(), _internals->frameIndex, _internals->rtvDescriptorSize);
		_internals->commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		_internals->renderPass.camera = camera;
		_internals->renderPass.framebuffer = camera->GetFramebuffer();
		_internals->renderPass.activeState = nullptr;
		_internals->renderPass.drawableHead = nullptr;

		_internals->renderPass.viewMatrix = camera->GetViewMatrix();
		_internals->renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		_internals->renderPass.projectionMatrix = camera->GetProjectionMatrix();
		_internals->renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		_internals->renderPass.projectionViewMatrix = _internals->renderPass.projectionMatrix * _internals->renderPass.viewMatrix;

		// Clear
		const Color &clearColor = camera->GetClearColor();
		_internals->commandList->ClearRenderTargetView(rtvHandle, &clearColor.r, 0, nullptr);

		// Create drawables
		function();

		// Draw
		D3D12Drawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(drawable);
			drawable = drawable->_next;
		}
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
			LockGuard<SpinLock> lock(_lock);
			library = _defaultShaders->GetObjectForKey<ShaderLibrary>(options);

			if(!library)
			{
				library = CreateShaderLibraryWithFile(RNCSTR(":RayneD3D12:/Shaders.json"), options);
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
		/*		for(Entity *ent : entities)
		{
		_commandList->SetPipelineState(ent->_model->_material->_pipelineState.Get());
		_commandList->IASetPrimitiveTopology(ent->_model->_mesh->_topology);
		_commandList->IASetVertexBuffers(0, 1, &(ent->_model->_mesh->_vertexBufferView));
		if(ent->_model->_mesh->_indexBuffer)
		{
		_commandList->IASetIndexBuffer(&(ent->_model->_mesh->_indexBufferView));
		_commandList->DrawIndexedInstanced(ent->_model->_mesh->_indexCount, 1, 0, 0, 0);
		}
		else
		{
		_commandList->DrawInstanced(ent->_model->_mesh->_vertexCount, 1, 0, 0);
		}
		}*/

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
