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

namespace RN
{
	RNDefineMeta(D3D12Renderer, Renderer)

	D3D12Renderer::D3D12Renderer(D3D12RendererDescriptor *descriptor, D3D12Device *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_mipMapTextures(new Array()),
		_defaultShaders(new Dictionary()),
		_srvCbvHeap{ nullptr, nullptr, nullptr },
		_submittedCommandLists(new Array()),
		_executedCommandLists(new Array())
	{
		ID3D12Device *underlyingDevice = device->GetDevice();

		{
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = 3;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			underlyingDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap));
			_rtvDescriptorSize = underlyingDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// Describe and create depth stencil view descriptor heap
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			underlyingDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap));

			_srvCbvDescriptorSize = underlyingDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// Create a root signature.
		{
			CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
			CD3DX12_ROOT_PARAMETER rootParameters[2];

			// Perfomance TIP: Order from most frequent to least frequent.
			srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 0);
			srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
			rootParameters[0].InitAsDescriptorTable(1, &srvCbvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[1], D3D12_SHADER_VISIBILITY_VERTEX);

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
	}

	D3D12Renderer::~D3D12Renderer()
	{
		
	}

	D3D12CommandList *D3D12Renderer::GetCommandList()
	{
		D3D12CommandList *commandList = new D3D12CommandList(GetD3D12Device()->GetDevice());
		return commandList->Autorelease();
	}

	D3D12CommandListWithCallback *D3D12Renderer::GetCommandListWithCallback()
	{
		D3D12CommandListWithCallback *commandList = new D3D12CommandListWithCallback(GetD3D12Device()->GetDevice());
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
		RN_ASSERT(!_mainWindow, "D3D12 renderer only supports one window");

		_mainWindow = new D3D12Window(size, screen, this);
		return _mainWindow;
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

		ShaderLibrary *library = CreateShaderLibraryWithFile(RNCSTR(":RayneD3D12:/Shaders.json"), nullptr);
		D3D12Shader *compute = library->GetShaderWithName(RNCSTR("GenerateMipMaps"))->Downcast<D3D12Shader>();
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


	void D3D12Renderer::RenderIntoWindow(Window *twindow, Function &&function)
	{
		D3D12Window *window = static_cast<D3D12Window *>(twindow);

		//Delete command lists that finished execution on the graphics card (the command allocator needs to be alive the whole time)
		for(int i = _executedCommandLists->GetCount() - 1; i >= 0; i--)
		{
			if(_executedCommandLists->GetObjectAtIndex<D3D12CommandList>(i)->_fenceValue <= window->GetCompletedFenceValue())
			{
				_executedCommandLists->RemoveObjectAtIndex(i);
			}
		}

		CreateMipMaps();

		window->AcquireBackBuffer();
		_currentCommandList = GetCommandList();

		_currentCommandList->GetCommandList()->SetGraphicsRootSignature(_rootSignature);
		
		function();

		ID3D12Resource *renderTarget = window->GetFramebuffer()->GetRenderTarget(window->GetFrameIndex());

		_currentCommandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		_currentCommandList->End();
		SubmitCommandList(_currentCommandList);
		_currentCommandList = nullptr;

		// Execute all command lists
		std::vector<ID3D12CommandList*> commandLists;
		_submittedCommandLists->Enumerate<D3D12CommandList>([&](D3D12CommandList *list, size_t index, bool &stop) {
			commandLists.push_back(list->GetCommandList());
			list->_fenceValue = window->GetCurrentFenceValue();
		});
		_executedCommandLists->AddObjectsFromArray(_submittedCommandLists);
		_submittedCommandLists->RemoveAllObjects();
		window->GetCommandQueue()->ExecuteCommandLists(commandLists.size(), &commandLists[0]);

		window->PresentBackBuffer();
	}

	void D3D12Renderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
		_internals->renderPass.drawableHead = nullptr;
		_internals->renderPass.drawableCount = 0;
		_internals->renderPass.textureDescriptorCount = 0;

		_internals->renderPass.viewMatrix = camera->GetViewMatrix();
		_internals->renderPass.inverseViewMatrix = camera->GetInverseViewMatrix();

		_internals->renderPass.projectionMatrix = camera->GetProjectionMatrix();
		//_internals->renderPass.projectionMatrix.m[5] *= -1.0f;
		_internals->renderPass.inverseProjectionMatrix = camera->GetInverseProjectionMatrix();

		_internals->renderPass.projectionViewMatrix = _internals->renderPass.projectionMatrix * _internals->renderPass.viewMatrix;

		// Create drawables
		function();

		CreateDescriptorHeap();

		D3D12Framebuffer *framebuffer = static_cast<D3D12Framebuffer *>(camera->GetFramebuffer());

		if(!framebuffer)
			framebuffer = _mainWindow->GetFramebuffer();

		ID3D12Resource *renderTarget = framebuffer->GetRenderTarget(_mainWindow->GetFrameIndex());

		Vector2 size = _mainWindow->GetSize();

		D3D12_VIEWPORT viewport;
		viewport.Width = size.x;
		viewport.Height = size.y;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MaxDepth = 1.0;
		viewport.MinDepth = 0.0;

		D3D12_RECT scissorRect;
		scissorRect.right = static_cast<LONG>(size.x);
		scissorRect.bottom = static_cast<LONG>(size.y);
		scissorRect.left = 0;
		scissorRect.top = 0;

		ID3D12GraphicsCommandList *commandList = _currentCommandList->GetCommandList();

		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		// Indicate that the back buffer will be used as a render target.
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _mainWindow->GetFrameIndex(), _rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(1, &rtvHandle, false , &dsvHandle);

		// Clear
		const Color &clearColor = camera->GetClearColor();
		commandList->ClearRenderTargetView(rtvHandle, &clearColor.r, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// Set the the one big descriptor heap for the whole frame
		ID3D12DescriptorHeap* srvCbvHeaps[] = { _srvCbvHeap[_mainWindow->GetFrameIndex()] };
		commandList->SetDescriptorHeaps(_countof(srvCbvHeaps), srvCbvHeaps);

		//Draw drawables
		D3D12Drawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
		{
			RenderDrawable(commandList, drawable);
			drawable = drawable->_next;
		}
	}

	GPUBuffer *D3D12Renderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		return new D3D12GPUBuffer(nullptr, length, usageOptions);
	}
	GPUBuffer *D3D12Renderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions)
	{
		return new D3D12GPUBuffer(bytes, length, usageOptions);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options)
	{
		return new D3D12ShaderLibrary(file);
	}

	ShaderLibrary *D3D12Renderer::CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options)
	{
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
			LockGuard<Lockable> lock(_lock);
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
/*		D3D12Drawable *newDrawable;
		if(_unusedDrawableIndices.size() > 0)
		{
			newDrawable = &_drawables[_unusedDrawableIndices.back()];
			_unusedDrawableIndices.pop_back();
			newDrawable->_index = _drawables.size() - 1;
		}
		else
		{
			D3D12Drawable drawable = D3D12Drawable();
			_drawables.push_back(D3D12Drawable());
			newDrawable = &_drawables.back();
		}
		
		newDrawable->_pipelineState = nullptr;
		newDrawable->_uniformState = nullptr;
		newDrawable->_active = true;

		return newDrawable;*/

		D3D12Drawable *newDrawable = new D3D12Drawable();
		newDrawable->_pipelineState = nullptr;
		newDrawable->_uniformState = nullptr;
		return newDrawable;
	}

	void D3D12Renderer::DeleteDrawable(Drawable *drawable)
	{
/*		D3D12Drawable *oldDrawable = static_cast<D3D12Drawable*>(drawable);
		_unusedDrawableIndices.push_back(oldDrawable->_index);
		oldDrawable->_active = false;*/

		delete drawable;
	}

	void D3D12Renderer::CreateDescriptorHeap()
	{
		_currentDrawableIndex = 0;
		ID3D12Device *device = GetD3D12Device()->GetDevice();

		if(_srvCbvHeap[_mainWindow->GetFrameIndex()])
		{
			_srvCbvHeap[_mainWindow->GetFrameIndex()]->Release();
		}

		// Layout: 5 textures + 1 cbv
		D3D12_DESCRIPTOR_HEAP_DESC srvCbvHeapDesc = {};
		srvCbvHeapDesc.NumDescriptors = (5 + 1)*_internals->renderPass.drawableCount;
		srvCbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvCbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&srvCbvHeapDesc, IID_PPV_ARGS(&_srvCbvHeap[_mainWindow->GetFrameIndex()]));

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

		CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle(_srvCbvHeap[_mainWindow->GetFrameIndex()]->GetCPUDescriptorHandleForHeapStart(), 0, _srvCbvDescriptorSize);

		D3D12Drawable *drawable = _internals->renderPass.drawableHead;
		while(drawable)
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
			D3D12UniformBuffer *uniformBuffer = drawable->_uniformState->uniformBuffer;
			D3D12GPUBuffer *actualBuffer = uniformBuffer->GetActiveBuffer()->Downcast<D3D12GPUBuffer>();
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = actualBuffer->GetD3D12Resource()->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = actualBuffer->GetLength();
			GetD3D12Device()->GetDevice()->CreateConstantBufferView(&cbvDesc, currentCPUHandle);
			currentCPUHandle.Offset(1, _srvCbvDescriptorSize);
			
			drawable = drawable->_next;
		}
	}

	void D3D12Renderer::FillUniformBuffer(D3D12UniformBuffer *uniformBuffer, D3D12Drawable *drawable)
	{
		GPUBuffer *gpuBuffer = uniformBuffer->Advance();
		uint8 *buffer = reinterpret_cast<uint8 *>(gpuBuffer->GetBuffer());

		Matrix result = _internals->renderPass.projectionViewMatrix * drawable->modelMatrix;
		std::memcpy(buffer, result.m, sizeof(Matrix));

		/*	const VulkanUniformBuffer::Member *member;*/

		// Matrices
		//		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelMatrix)))
		{
			std::memcpy(buffer + sizeof(Matrix), drawable->modelMatrix.m, sizeof(Matrix));
		}
		/*		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::ModelViewMatrix)))
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
		}*/

		// Color
		Material *material = drawable->material;

		//		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::AmbientColor)))
		{
			std::memcpy(buffer + sizeof(Matrix) * 2, &material->GetAmbientColor().r, sizeof(Color));
		}
		//		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::DiffuseColor)))
		{
			std::memcpy(buffer + sizeof(Matrix) * 2 + sizeof(Color), &material->GetDiffuseColor().r, sizeof(Color));
		}
		/*		if((member = uniformBuffer->GetMemberForFeature(MetalUniformBuffer::Feature::SpecularColor)))
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
		{*/
		float temp = material->GetTextureTileFactor();
		std::memcpy(buffer + sizeof(Matrix) * 2 + sizeof(Color) * 2, &temp, sizeof(float));
		//}

		gpuBuffer->Invalidate();
	}

	void D3D12Renderer::SubmitDrawable(Drawable *tdrawable)
	{
		D3D12Drawable *drawable = static_cast<D3D12Drawable *>(tdrawable);

		if(drawable->dirty)
		{
			//TODO: Fix the camera situation...
			_lock.Lock();
			const D3D12PipelineState *pipelineState = _internals->stateCoordinator.GetRenderPipelineState(drawable->material, drawable->mesh, nullptr);
			D3D12UniformState *uniformState = _internals->stateCoordinator.GetUniformStateForPipelineState(pipelineState, drawable->material);
			_lock.Unlock();

			drawable->UpdateRenderingState(this, pipelineState, uniformState);
			drawable->dirty = false;
		}

		FillUniformBuffer(drawable->_uniformState->uniformBuffer, drawable);

		// Push into the queue
		drawable->_prev = nullptr;

		_lock.Lock();

		drawable->_next = _internals->renderPass.drawableHead;

		if(drawable->_next)
			drawable->_next->_prev = drawable;

		_internals->renderPass.drawableHead = drawable;
		_internals->renderPass.drawableCount++;

		_lock.Unlock();
	}

	void D3D12Renderer::RenderDrawable(ID3D12GraphicsCommandList *commandList, D3D12Drawable *drawable)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGPUHandle(_srvCbvHeap[_mainWindow->GetFrameIndex()]->GetGPUDescriptorHandleForHeapStart(), _currentDrawableIndex*6 + 5, _srvCbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(1, cbvGPUHandle);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGPUHandle(_srvCbvHeap[_mainWindow->GetFrameIndex()]->GetGPUDescriptorHandleForHeapStart(), _currentDrawableIndex*6, _srvCbvDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(0, srvGPUHandle);

		commandList->SetPipelineState(drawable->_pipelineState->state);

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
