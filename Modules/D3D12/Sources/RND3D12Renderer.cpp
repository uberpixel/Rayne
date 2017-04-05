//
//  RND3D12Renderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12Renderer.h"
#include "RND3D12ShaderLibrary.h"
#include "RND3D12GPUBuffer.h"
#include "RND3D12Texture.h"
#include "RND3D12UniformBuffer.h"
#include "RND3D12Device.h"
#include "RND3D12Framebuffer.h"
#include "RND3D12Internals.h"
#include "RND3D12SwapChain.h"

namespace RN
{
	RNDefineMeta(D3D12Renderer, Renderer)

	D3D12Renderer::D3D12Renderer(D3D12RendererDescriptor *descriptor, D3D12Device *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_mipMapTextures(new Array()),
		_currentSrvCbvHeap(nullptr),
		_submittedCommandLists(new Array()),
		_executedCommandLists(new Array()),
		_commandListPool(new Array()),
		_scheduledFenceValue(0),
		_completedFenceValue(0)
	{
		ID3D12Device *underlyingDevice = device->GetDevice();

		underlyingDevice->CreateFence(_scheduledFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

		D3D12_COMMAND_QUEUE_DESC queueDescriptor = {};
		queueDescriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDescriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		underlyingDevice->CreateCommandQueue(&queueDescriptor, IID_PPV_ARGS(&_commandQueue));

		_rtvDescriptorSize = underlyingDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_srvCbvDescriptorSize = underlyingDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Create a root signature.
		{
			CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
			CD3DX12_ROOT_PARAMETER rootParameters[2];

			// Perfomance TIP: Order from most frequent to least frequent.
			srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0);
			srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
			rootParameters[0].InitAsDescriptorTable(1, &srvCbvRanges[0], D3D12_SHADER_VISIBILITY_ALL);	//TODO: Restrict visibility to the shader actually using it
			rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[1], D3D12_SHADER_VISIBILITY_ALL);	//TODO: Restrict visibility to the shader actually using it

			// Create sampler
			D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;//D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			samplerDesc.MinLOD = 0.0f;
			samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			samplerDesc.MaxAnisotropy = 16;
			samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			samplerDesc.ShaderRegister = 0;
			samplerDesc.RegisterSpace = 0;
			samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ID3DBlob *signature;
			ID3DBlob *error;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
			underlyingDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
		}

		_defaultShaderLibrary = CreateShaderLibraryWithFile(RNCSTR(":RayneD3D12:/Shaders.json"));
	}

	D3D12Renderer::~D3D12Renderer()
	{
		
	}

	D3D12CommandList *D3D12Renderer::GetCommandList()
	{
		D3D12CommandList *commandList = nullptr;
		if(_commandListPool->GetCount() == 0)
		{
			commandList = new D3D12CommandList(GetD3D12Device()->GetDevice());
		}
		else
		{
			commandList = _commandListPool->GetLastObject<D3D12CommandList>();
			commandList->Retain();
			_commandListPool->RemoveObjectAtIndex(_commandListPool->GetCount() - 1);
			commandList->Begin();
		}
		
		return commandList->Autorelease();
	}

	void D3D12Renderer::SubmitCommandList(D3D12CommandList *commandList)
	{
		_lock.Lock();
		_submittedCommandLists->AddObject(commandList);
		_lock.Unlock();
	}

	Window *D3D12Renderer::CreateAWindow(const Vector2 &size, Screen *screen)
	{
		D3D12Window *window = new D3D12Window(size, screen, this, 4);

		if(!_mainWindow)
			_mainWindow = window;

		return window;
	}

	Window *D3D12Renderer::GetMainWindow()
	{
		return _mainWindow;
	}


	void D3D12Renderer::CreateMipMapsForTexture(D3D12Texture *texture)
	{
		_lock.Lock();
		_mipMapTextures->AddObject(texture);
		_lock.Unlock();
	}

	void D3D12Renderer::CreateMipMaps()
	{
		if(_mipMapTextures->GetCount() == 0)
			return;

		//Calculate heap size
		uint32 requiredHeapSize = 0;
		bool waitingForUploads = false;
		_mipMapTextures->Enumerate<D3D12Texture>([&](D3D12Texture *texture, size_t index, bool &stop) {
			if(texture->GetDescriptor().mipMaps > 1)
				requiredHeapSize += texture->GetDescriptor().mipMaps - 1;

			if(!texture->_isReady)
			{
				waitingForUploads = true;
				stop = true;
			}
		});

		if(waitingForUploads)
			return;

		if(requiredHeapSize == 0)
		{
			_mipMapTextures->RemoveAllObjects();
			return;
		}

		struct DWParam
		{
			DWParam(FLOAT f) : Float(f) {}
			DWParam(UINT u) : Uint(u) {}
			DWParam(INT i) : Int(i) {}

			void operator= (FLOAT f) { Float = f; }
			void operator= (UINT u) { Uint = u; }
			void operator= (INT i) { Int = i; }

			union
			{
				FLOAT Float;
				UINT Uint;
				INT Int;
			};
		};

		CD3DX12_DESCRIPTOR_RANGE descriptorRanges[2];
		CD3DX12_ROOT_PARAMETER rootParameters[3];

		// Perfomance TIP: Order from most frequent to least frequent.
		descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		rootParameters[0].InitAsConstants(2, 0);
		rootParameters[1].InitAsDescriptorTable(1, &descriptorRanges[0]);
		rootParameters[2].InitAsDescriptorTable(1, &descriptorRanges[1]);

		// Create sampler
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MaxAnisotropy = 16;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob *signature;
		ID3DBlob *error;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		ID3D12RootSignature *mipMapRootSignature;
		GetD3D12Device()->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mipMapRootSignature));

		// Layout: source texture - destination texture
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 2*requiredHeapSize;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ID3D12DescriptorHeap *descriptorHeap;
		GetD3D12Device()->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap));

		D3D12Shader *compute = _defaultShaderLibrary->GetShaderWithName(RNCSTR("GenerateMipMaps"))->Downcast<D3D12Shader>();
		ID3DBlob *computeShader = static_cast<ID3DBlob*>(compute->_shader);

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mipMapRootSignature;
		psoDesc.CS = { reinterpret_cast<UINT8*>(computeShader->GetBufferPointer()), computeShader->GetBufferSize() };

		ID3D12PipelineState *psoMipMaps;
		GetD3D12Device()->GetDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&psoMipMaps));


		// now we create a shader resource view (descriptor that points to the texture and describes it)
		D3D12_SHADER_RESOURCE_VIEW_DESC textureSrvDesc = {};
		textureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		textureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		D3D12_UNORDERED_ACCESS_VIEW_DESC textureUavDesc = {};
		textureUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		D3D12CommandList *commandList = GetCommandList();
		ID3D12GraphicsCommandList *d3d12CommandList = commandList->GetCommandList();
		d3d12CommandList->SetComputeRootSignature(mipMapRootSignature);
		d3d12CommandList->SetPipelineState(psoMipMaps);
		d3d12CommandList->SetDescriptorHeaps(1, &descriptorHeap);

		CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, _srvCbvDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE currentGPUHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, _srvCbvDescriptorSize);

		_mipMapTextures->Enumerate<D3D12Texture>([&](D3D12Texture *texture, size_t index, bool &stop) {
			Texture::Descriptor textureDescriptor = texture->GetDescriptor();
			if(textureDescriptor.mipMaps <= 1)
				return;

			texture->TransitionToState(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			for(uint32_t TopMip = 0; TopMip < textureDescriptor.mipMaps-1; TopMip++)
			{
				uint32_t dstWidth = std::max(textureDescriptor.width >> (TopMip+1), 1u);
				uint32_t dstHeight = std::max(textureDescriptor.height >> (TopMip+1), 1u);

				textureSrvDesc.Format = texture->_format;
				textureSrvDesc.Texture2D.MipLevels = 1;
				textureSrvDesc.Texture2D.MostDetailedMip = TopMip;
				GetD3D12Device()->GetDevice()->CreateShaderResourceView(texture->_textureBuffer, &textureSrvDesc, currentCPUHandle);
				currentCPUHandle.Offset(1, _srvCbvDescriptorSize);

				textureUavDesc.Format = texture->_format;
				textureUavDesc.Texture2D.MipSlice = TopMip+1;
				GetD3D12Device()->GetDevice()->CreateUnorderedAccessView(texture->_textureBuffer, nullptr, &textureUavDesc, currentCPUHandle);
				currentCPUHandle.Offset(1, _srvCbvDescriptorSize);

				d3d12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f/ dstWidth).Uint, 0);
				d3d12CommandList->SetComputeRoot32BitConstant(0, DWParam(1.0f/ dstHeight).Uint, 1);
				
				d3d12CommandList->SetComputeRootDescriptorTable(1, currentGPUHandle);
				currentGPUHandle.Offset(1, _srvCbvDescriptorSize);
				d3d12CommandList->SetComputeRootDescriptorTable(2, currentGPUHandle);
				currentGPUHandle.Offset(1, _srvCbvDescriptorSize);
				d3d12CommandList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);
				d3d12CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(texture->_textureBuffer));
			}

			texture->TransitionToState(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			texture->_needsMipMaps = false;
		});

		commandList->End();
		SubmitCommandList(commandList);

		_mipMapTextures->RemoveAllObjects();
	}


	void D3D12Renderer::Render(Function &&function)
	{
		_currentDrawableIndex = 0;
		_internals->renderPasses.clear();
		_internals->totalDrawableCount = 0;
		_internals->currentCameraID = 0;
		_internals->swapChains.clear();

		_completedFenceValue = _fence->GetCompletedValue();

		//Delete command lists that finished execution on the graphics card (the command allocator needs to be alive the whole time)
		for(int i = _executedCommandLists->GetCount() - 1; i >= 0; i--)
		{
			if(_executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i)->_fenceValue <= _completedFenceValue)
			{
				D3D12CommandList *commandList = _executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i);
				commandList->Finish();
				_commandListPool->AddObject(commandList);
				_executedCommandLists->RemoveObjectAtIndex(i);
			}
		}

		//Free other frame resources such as descriptor heaps, that are not in use by the gpu anymore
		for(int i = _internals->frameResources.size()-1; i >= 0; i--)
		{
			D3D12FrameResource &frameResource = _internals->frameResources[i];
			if(frameResource.frame <= _completedFenceValue)
			{
				frameResource.resource->Release();
				_internals->frameResources.erase(_internals->frameResources.begin() + i);
			}
		}

		CreateMipMaps();		

		//RenderIntoCamera is called for each camera and creates lists of drawables per camera
		function();

		for(D3D12SwapChain *swapChain : _internals->swapChains)
		{
			swapChain->AcquireBackBuffer();
		}

		CreateDescriptorHeap();

		_currentCommandList = GetCommandList();

		for(D3D12SwapChain *swapChain : _internals->swapChains)
		{
			swapChain->Prepare(_currentCommandList);
		}

		ID3D12GraphicsCommandList *commandList = _currentCommandList->GetCommandList();
		commandList->SetGraphicsRootSignature(_rootSignature);

		// Set the one big descriptor heap for the whole frame
		ID3D12DescriptorHeap* srvCbvHeaps[] = { _currentSrvCbvHeap };
		commandList->SetDescriptorHeaps(_countof(srvCbvHeaps), srvCbvHeaps);
		
		_internals->currentCameraID = 0;
		for(D3D12RenderPass renderPass : _internals->renderPasses)
		{
			SetCameraAsRendertarget(_currentCommandList, renderPass.camera);

			if(renderPass.drawables.size() > 0)
			{
				//Draw drawables
				for(D3D12Drawable *drawable : renderPass.drawables)
				{
					RenderDrawable(commandList, drawable);
				}

				_internals->currentCameraID += 1;
			}
		}

		for(D3D12SwapChain *swapChain : _internals->swapChains)
		{
			swapChain->Finalize(_currentCommandList);
		}
		
		_currentCommandList->End();
		SubmitCommandList(_currentCommandList);
		_currentCommandList = nullptr;

		// Execute all command lists
		std::vector<ID3D12CommandList*> commandLists;
		_submittedCommandLists->Enumerate<D3D12CommandList>([&](D3D12CommandList *list, size_t index, bool &stop) {
			commandLists.push_back(list->GetCommandList());
			list->_fenceValue = _scheduledFenceValue;
		});
		_executedCommandLists->AddObjectsFromArray(_submittedCommandLists);
		_submittedCommandLists->RemoveAllObjects();
		_commandQueue->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

		_commandQueue->Signal(_fence, _scheduledFenceValue++);

		for(D3D12SwapChain *swapChain : _internals->swapChains)
		{
			swapChain->PresentBackBuffer();
		}
	}

	void D3D12Renderer::SubmitCamera(Camera *camera, Function &&function)
	{
		_internals->currentRenderPass.camera = camera;
		_internals->currentRenderPass.drawables.resize(0);

		_internals->currentRenderPass.viewMatrix = camera->GetViewMatrix();
		_internals->currentRenderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		_internals->currentRenderPass.projectionMatrix = camera->GetProjectionMatrix();
		//renderPass.projectionMatrix.m[5] *= -1.0f;
		_internals->currentRenderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		_internals->currentRenderPass.projectionViewMatrix = _internals->currentRenderPass.projectionMatrix * _internals->currentRenderPass.viewMatrix;


		Framebuffer *framebuffer = camera->GetFramebuffer();
		D3D12SwapChain *newSwapChain = nullptr;
		if(!framebuffer)
		{
			D3D12Window *window = static_cast<D3D12Window *>(_mainWindow);
			D3D12SwapChain *swapChain = window->GetSwapChain();
			newSwapChain = swapChain;
		}
		else
		{
			newSwapChain = framebuffer->Downcast<D3D12Framebuffer>()->GetSwapChain();
		}
		
		if(newSwapChain)
		{
			bool notIncluded = true;
			for(D3D12SwapChain *swapChain : _internals->swapChains)
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


		// Create drawables
		function();

		_internals->totalDrawableCount += _internals->currentRenderPass.drawables.size();
		_internals->renderPasses.push_back(_internals->currentRenderPass);
		_internals->currentCameraID += 1;
	}

	GPUBuffer *D3D12Renderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		return new D3D12GPUBuffer(nullptr, length, usageOptions);
	}
	GPUBuffer *D3D12Renderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		return new D3D12GPUBuffer(bytes, length, usageOptions);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithFile(const String *file)
	{
		return new D3D12ShaderLibrary(file);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithSource(const String *source)
	{
		D3D12ShaderLibrary *lib = new D3D12ShaderLibrary(nullptr);
		return lib;
	}

	Shader *D3D12Renderer::GetDefaultShader(Shader::Type type, ShaderOptions *options, Shader::Default shader)
	{
		Shader *shader;
		if(type == Shader::Type::Vertex)
		{
			if(default == Shader::Default::Sky)
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("sky_vertex"), options);
			}
			else
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("gouraud_vertex"), options);
			}
		}
			
		else if(type == Shader::Type::Fragment)
		{
			if(default == Shader::Default::Sky)
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("sky_fragment"), options);
			}
			else
			{
				shader = _defaultShaderLibrary->GetShaderWithName(RNCSTR("gouraud_fragment"), options);
			}
		}

		return shader;
	}

	bool D3D12Renderer::SupportsTextureFormat(const String *format) const
	{
		//TODO: Fix this
		return true;
	}

	bool D3D12Renderer::SupportsDrawMode(DrawMode mode) const
	{
		return true;
	}

	size_t D3D12Renderer::GetAlignmentForType(PrimitiveType type) const
	{
		// TODO!!!!
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

			default:
				return 1;
		}
	}

	size_t D3D12Renderer::GetSizeForType(PrimitiveType type) const
	{
		// TODO!!!
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

			default:
				return 1;
		}
	}

	Texture *D3D12Renderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		return new D3D12Texture(descriptor, this);
	}

	Framebuffer *D3D12Renderer::CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor)
	{
		return nullptr;
	}

	Drawable *D3D12Renderer::CreateDrawable()
	{
		D3D12Drawable *newDrawable = new D3D12Drawable();
		return newDrawable;
	}

	void D3D12Renderer::DeleteDrawable(Drawable *drawable)
	{
		delete drawable;
	}

	void D3D12Renderer::SetCameraAsRendertarget(D3D12CommandList *commandList, Camera *camera)
	{
		ID3D12Device *underlyingDevice = GetD3D12Device()->GetDevice();
		ID3D12GraphicsCommandList *d3dCommandList = commandList->GetCommandList();

		//Get the cameras frame buffer
		D3D12Framebuffer *framebuffer = static_cast<D3D12Framebuffer *>(camera->GetFramebuffer());
		if(!framebuffer)
		{
			D3D12Window *window = static_cast<D3D12Window *>(_mainWindow);
			D3D12SwapChain *swapChain = window->GetSwapChain();
			framebuffer = swapChain->GetFramebuffer();
		}

		ID3D12DescriptorHeap *rtvHeap;
		ID3D12DescriptorHeap *dsvHeap;

		//TODO: Create heaps per framebuffer and not per camera
		//Create new heaps
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		underlyingDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		underlyingDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

		_internals->frameResources.push_back({ rtvHeap, _scheduledFenceValue });
		_internals->frameResources.push_back({ dsvHeap, _scheduledFenceValue });

		//TODO: Don't hardcode these descriptors!
		//Create the render target view
		D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
		renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		underlyingDevice->CreateRenderTargetView(framebuffer->GetRenderTarget(), &renderTargetViewDesc, rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
		underlyingDevice->CreateDepthStencilView(framebuffer->GetDepthBuffer(), &depthStencilViewDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

		//Set the rendertargets
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
		d3dCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		//Setup viewport and scissor rect
		Rect cameraRect = camera->GetFrame();
		if(cameraRect.width < 0.5f || cameraRect.height < 0.5f)
		{
			Vector2 framebufferSize = framebuffer->GetSize();
			cameraRect.x = 0.0f;
			cameraRect.y = 0.0f;
			cameraRect.width = framebufferSize.x;
			cameraRect.height = framebufferSize.y;
		}

		D3D12_VIEWPORT viewport;
		viewport.Width = cameraRect.width;
		viewport.Height = cameraRect.height;
		viewport.TopLeftX = cameraRect.x;
		viewport.TopLeftY = cameraRect.y;
		viewport.MaxDepth = 1.0;
		viewport.MinDepth = 0.0;

		D3D12_RECT scissorRect;
		scissorRect.right = static_cast<LONG>(cameraRect.width + cameraRect.x);
		scissorRect.bottom = static_cast<LONG>(cameraRect.height + cameraRect.y);
		scissorRect.left = cameraRect.x;
		scissorRect.top = cameraRect.y;

		d3dCommandList->RSSetViewports(1, &viewport);
		d3dCommandList->RSSetScissorRects(1, &scissorRect);

		// Clear
		//TODO: It would probably be better to just clear the whole rendertarget at once...
		const Color &clearColor = camera->GetClearColor();
		d3dCommandList->ClearRenderTargetView(rtvHandle, &clearColor.r, 1, &scissorRect);
		d3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 1, &scissorRect);
	}

	void D3D12Renderer::CreateDescriptorHeap()
	{
		if(_internals->totalDrawableCount == 0)
			return;

		ID3D12Device *device = GetD3D12Device()->GetDevice();

		ID3D12DescriptorHeap *srvCbvHeap;

		// Layout: 5 textures + 1 cbv
		D3D12_DESCRIPTOR_HEAP_DESC srvCbvHeapDesc = {};
		srvCbvHeapDesc.NumDescriptors = (5 + 1)*_internals->totalDrawableCount;
		srvCbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvCbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&srvCbvHeapDesc, IID_PPV_ARGS(&srvCbvHeap));
		_currentSrvCbvHeap = srvCbvHeap;
		_internals->frameResources.push_back({ srvCbvHeap, _scheduledFenceValue });

		// Describe null SRVs. Null descriptors are used to "unbind" textures
		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		nullSrvDesc.Texture2D.MipLevels = 1;
		nullSrvDesc.Texture2D.MostDetailedMip = 0;
		nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		// now we create a shader resource view (descriptor that points to the texture and describes it)
		D3D12_SHADER_RESOURCE_VIEW_DESC textureSrvDesc = {};
		textureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		textureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

		CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle(srvCbvHeap->GetCPUDescriptorHandleForHeapStart(), 0, _srvCbvDescriptorSize);

		size_t cameraID = 0;
		for(D3D12RenderPass renderPass : _internals->renderPasses)
		{
			if(renderPass.drawables.size() == 0)
				continue;

			for(D3D12Drawable *drawable : renderPass.drawables)
			{
				//Create texture descriptors
				const Array *textures = drawable->material->GetTextures();
				textures->Enumerate<D3D12Texture>([&](D3D12Texture *texture, size_t i, bool &stop) {
					//Root signature expects 5 textures max
					if(i >= 5)
					{
						stop = true;
						return;
					}

					//Check if texture finished uploading to the vram
					if(texture->_isReady && !texture->_needsMipMaps)
					{
						textureSrvDesc.Format = texture->_format;
						textureSrvDesc.Texture2D.MipLevels = texture->GetDescriptor().mipMaps;
						device->CreateShaderResourceView(texture->_textureBuffer, &textureSrvDesc, currentCPUHandle);
						currentCPUHandle.Offset(1, _srvCbvDescriptorSize);
					}
					else
					{
						device->CreateShaderResourceView(nullptr, &nullSrvDesc, currentCPUHandle);
						currentCPUHandle.Offset(1, _srvCbvDescriptorSize);
					}
				});

				//Create null texture descriptors for those textures not used within the limit of 5
				for(int i = 5 - textures->GetCount(); i > 0; i--)
				{
					device->CreateShaderResourceView(nullptr, &nullSrvDesc, currentCPUHandle);
					currentCPUHandle.Offset(1, _srvCbvDescriptorSize);
				}

				//Create constant buffer descriptor
				D3D12UniformBuffer *uniformBuffer = drawable->_cameraSpecifics[cameraID].uniformState->uniformBuffer;
				D3D12GPUBuffer *actualBuffer = uniformBuffer->GetActiveBuffer()->Downcast<D3D12GPUBuffer>();

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = actualBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
				cbvDesc.SizeInBytes = actualBuffer->GetLength();
				GetD3D12Device()->GetDevice()->CreateConstantBufferView(&cbvDesc, currentCPUHandle);
				currentCPUHandle.Offset(1, _srvCbvDescriptorSize);
			}

			cameraID += 1;
		}
	}

	void D3D12Renderer::FillUniformBuffer(uint8 *buffer, D3D12Drawable *drawable, Shader *shader, size_t &offset)
	{
		Material *material = drawable->material;

		buffer += offset;

		shader->GetSignature()->GetUniformDescriptors()->Enumerate<Shader::UniformDescriptor>([&](Shader::UniformDescriptor *descriptor, size_t index, bool &stop) {
			offset = descriptor->GetOffset() + descriptor->GetSize();

			switch(descriptor->GetIdentifier())
			{
				case Shader::UniformDescriptor::Identifier::ModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->modelMatrix.m, descriptor->GetSize());
					break;
				}
				
				case Shader::UniformDescriptor::Identifier::ModelViewMatrix:
				{
					Matrix result = _internals->currentRenderPass.viewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ModelViewProjectionMatrix:
				{
					Matrix result = _internals->currentRenderPass.projectionViewMatrix * drawable->modelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), _internals->currentRenderPass.viewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), _internals->currentRenderPass.projectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::ProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), _internals->currentRenderPass.projectionMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), drawable->inverseModelMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewMatrix:
				{
					Matrix result = _internals->currentRenderPass.inverseViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseModelViewProjectionMatrix:
				{
					Matrix result = _internals->currentRenderPass.inverseProjectionViewMatrix * drawable->inverseModelMatrix;
					std::memcpy(buffer + descriptor->GetOffset(), result.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), _internals->currentRenderPass.inverseViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseViewProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), _internals->currentRenderPass.inverseProjectionViewMatrix.m, descriptor->GetSize());
					break;
				}

				case Shader::UniformDescriptor::Identifier::InverseProjectionMatrix:
				{
					std::memcpy(buffer + descriptor->GetOffset(), _internals->currentRenderPass.inverseProjectionMatrix.m, descriptor->GetSize());
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

	void D3D12Renderer::SubmitDrawable(Drawable *tdrawable)
	{
		D3D12Drawable *drawable = static_cast<D3D12Drawable *>(tdrawable);
		drawable->AddUniformStateIfNeeded(_internals->currentCameraID);

		if(drawable->dirty)
		{
			//TODO: Fix the camera situation...
			_lock.Lock();
			const D3D12PipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);
			D3D12UniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState, drawable->material);
			_lock.Unlock();

			drawable->UpdateRenderingState(_internals->currentCameraID, pipelineState, uniformState);
			drawable->dirty = false;
		}

		GPUBuffer *gpuBuffer = drawable->_cameraSpecifics[_internals->currentCameraID].uniformState->uniformBuffer->Advance();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());
		size_t offset = 0;
		FillUniformBuffer(buffer, drawable, drawable->material->GetVertexShader(), offset);
		FillUniformBuffer(buffer, drawable, drawable->material->GetFragmentShader(), offset);
		gpuBuffer->Invalidate();

		// Push into the queue
		_lock.Lock();
		_internals->currentRenderPass.drawables.push_back(drawable);
		_lock.Unlock();
	}

	void D3D12Renderer::RenderDrawable(ID3D12GraphicsCommandList *commandList, D3D12Drawable *drawable)
	{
		D3D12SwapChain *swapChain = _mainWindow->GetSwapChain();

		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGPUHandle(_currentSrvCbvHeap->GetGPUDescriptorHandleForHeapStart(), _currentDrawableIndex*6 + 5, _srvCbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(1, cbvGPUHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGPUHandle(_currentSrvCbvHeap->GetGPUDescriptorHandleForHeapStart(), _currentDrawableIndex*6, _srvCbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(0, srvGPUHandle);

		commandList->SetPipelineState(drawable->_cameraSpecifics[_internals->currentCameraID].pipelineState->state);

		D3D12GPUBuffer *buffer = static_cast<D3D12GPUBuffer *>(drawable->mesh->GetVertexBuffer());
		D3D12GPUBuffer *indices = static_cast<D3D12GPUBuffer *>(drawable->mesh->GetIndicesBuffer());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = buffer->GetD3D12Resource()->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = drawable->mesh->GetStride();
		vertexBufferView.SizeInBytes = buffer->GetLength();
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = indices->GetD3D12Resource()->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		indexBufferView.SizeInBytes = indices->GetLength();
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(drawable->mesh->GetIndicesCount(), 1, 0, 0, 0);

		_currentDrawableIndex += 1;
	}
}
