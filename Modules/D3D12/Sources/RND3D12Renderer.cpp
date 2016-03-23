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
#include "RND3D12RendererDescriptor.h"
#include "RND3D12Framebuffer.h"

namespace RN
{
	RNDefineMeta(D3D12Renderer, Renderer)

	D3D12Renderer::D3D12Renderer(D3D12RendererDescriptor *descriptor, D3D12Device *device) :
		Renderer(descriptor, device),
		_mainWindow(nullptr),
		_mipMapTextures(new Set()),
		_defaultShaders(new Dictionary()),
		_textureFormatLookup(new Dictionary())
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

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
			D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			underlyingDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&_cbvHeap));
			_cbvDescriptorSize = underlyingDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// Create an empty root signature.
		{
			CD3DX12_DESCRIPTOR_RANGE ranges[1];
			CD3DX12_ROOT_PARAMETER rootParameters[1];

			ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ID3DBlob *signature;
			ID3DBlob *error;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
			underlyingDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
		}

		// Texture format look ups
#define TextureFormat(name, d3d12) \
			case Texture::Format::name: { \
				_textureFormatLookup->SetObjectForKey(Number::WithUint32(d3d12), RNCSTR(#name)); \
			} break

		bool isDone = false;

		for(size_t i = 0; isDone == false; i++)
		{
			switch(static_cast<Texture::Format>(i))
			{
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
				TextureFormat(Depth24Stencil8, DXGI_FORMAT_D24_UNORM_S8_UINT);
				TextureFormat(Depth32FStencil8, DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

				case Texture::Format::RGB888:
				case Texture::Format::RGB16F:
				case Texture::Format::RGB32F:
				case Texture::Format::Stencil8:
					break;

				case Texture::Format::Invalid:
					isDone = true;
					break;
			}
		}
	}

	D3D12Renderer::~D3D12Renderer()
	{
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

		_mipMapTextures->RemoveAllObjects();
	}


	void D3D12Renderer::RenderIntoWindow(Window *twindow, Function &&function)
	{
		D3D12Window *window = static_cast<D3D12Window *>(twindow);

		window->AcquireBackBuffer();
		window->GetCommandAllocator()->Reset();
		window->GetCommandList()->Reset(window->GetCommandAllocator(), nullptr);

		window->GetCommandList()->SetGraphicsRootSignature(_rootSignature);

		ID3D12DescriptorHeap* ppHeaps[] = { _cbvHeap };
		window->GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		window->GetCommandList()->SetGraphicsRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart());
		
		function();

		ID3D12Resource *renderTarget = window->GetFramebuffer()->GetRenderTarget(window->GetFrameIndex());

		window->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		window->GetCommandList()->Close();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { window->GetCommandList() };
		window->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		window->PresentBackBuffer();
	}

	void D3D12Renderer::RenderIntoCamera(Camera *camera, Function &&function)
	{
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

		ID3D12GraphicsCommandList *commandList = _mainWindow->GetCommandList();

		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		// Indicate that the back buffer will be used as a render target.
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _mainWindow->GetFrameIndex(), _rtvDescriptorSize);
		commandList->OMSetRenderTargets(1, &rtvHandle, false , nullptr);

		// Clear
		const Color &clearColor = camera->GetClearColor();
		commandList->ClearRenderTargetView(rtvHandle, &clearColor.r, 0, nullptr);

		// Create drawables
		function();

		// Draw
	}

	GPUBuffer *D3D12Renderer::CreateBufferWithLength(size_t length, GPUResource::UsageOptions options)
	{
/*		MTLResourceOptions resourceOptions = D3D12ResourceOptionsFromOptions(options);*/
		return new D3D12GPUBuffer(nullptr, length);
	}
	GPUBuffer *D3D12Renderer::CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options)
	{
/*		MTLResourceOptions resourceOptions = D3D12ResourceOptionsFromOptions(options);*/
		return new D3D12GPUBuffer(bytes, length);
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
			TextureFormatX(RGB888);

			TextureFormatX(R16F);
			TextureFormatX(RG16F);
			TextureFormatX(RGB16F);
			TextureFormatX(RGBA16F);

			TextureFormatX(R32F);
			TextureFormatX(RG32F);
			TextureFormatX(RGB32F);
			TextureFormatX(RGBA32F);

			TextureFormatX(Depth24I);
			TextureFormatX(Depth32F);
			TextureFormatX(Stencil8);
			TextureFormatX(Depth24Stencil8);
			TextureFormatX(Depth32FStencil8);

			case Texture::Format::Invalid:
				return nullptr;
		}

#undef TextureFormatX

		return nullptr;
	}

	Texture *D3D12Renderer::CreateTextureWithDescriptor(const Texture::Descriptor &descriptor)
	{
		return new D3D12Texture(this, nullptr, nullptr, descriptor);
	}

	Framebuffer *D3D12Renderer::CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor)
	{
		return nullptr;
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
	}

	void D3D12Renderer::SubmitDrawable(Drawable *tdrawable)
	{
	}

	void D3D12Renderer::RenderDrawable(D3D12Drawable *drawable)
	{
	}
}
