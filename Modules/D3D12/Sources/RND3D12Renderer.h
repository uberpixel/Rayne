//
//  RND3D12Renderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12RENDERER_H__
#define __RAYNE_D3D12RENDERER_H__

#include "RND3D12.h"
#include "RND3D12Window.h"
#include "RND3D12Device.h"
#include "RND3D12RendererDescriptor.h"
#include "RND3D12StateCoordinator.h"
#include "RND3D12UniformBuffer.h"

namespace RN
{
	struct D3D12Drawable;
	class D3D12RendererInternals;
	class D3D12Window;
	class D3D12Texture;
	class D3D12UniformBuffer;

	struct D3D12Drawable : public Drawable
	{
		~D3D12Drawable()
		{
			for(D3D12UniformBuffer *buffer : _vertexBuffers)
				delete buffer;
			for(D3D12UniformBuffer *buffer : _fragmentBuffers)
				delete buffer;
		}

		void UpdateRenderingState(Renderer *renderer, const D3D12RenderingState *state)
		{
			if(state == _pipelineState)
				return;

			_pipelineState = state;

			for(D3D12UniformBuffer *buffer : _vertexBuffers)
				delete buffer;
			for(D3D12UniformBuffer *buffer : _fragmentBuffers)
				delete buffer;

			_vertexBuffers.clear();
			_fragmentBuffers.clear();

			for(D3D12RenderingStateArgument *argument : state->vertexArguments)
			{
				switch(argument->type)
				{
					case D3D12RenderingStateArgument::Type::Buffer:
					{
						if(argument->index > 0)
							_vertexBuffers.push_back(new D3D12UniformBuffer(renderer, static_cast<D3D12RenderingStateUniformBufferArgument *>(argument)));
					}

					default:
						break;
				}
			}

			for(D3D12RenderingStateArgument *argument : state->fragmentArguments)
			{
				switch(argument->type)
				{
					case D3D12RenderingStateArgument::Type::Buffer:
					{
						if(argument->index > 0)
							_fragmentBuffers.push_back(new D3D12UniformBuffer(renderer, static_cast<D3D12RenderingStateUniformBufferArgument *>(argument)));
					}

					default:
						break;
				}
			}
		}

		const D3D12RenderingState *_pipelineState;
		std::vector<D3D12UniformBuffer *> _vertexBuffers;
		std::vector<D3D12UniformBuffer *> _fragmentBuffers;
		D3D12Drawable *_next;
		D3D12Drawable *_prev;
	};

	class D3D12Renderer : public Renderer
	{
	public:
		friend class D3D12Texture;
		friend class D3D12Window;
		friend class D3D12StateCoordinator;
		friend class D3D12GPUBuffer;

		D3DAPI D3D12Renderer(D3D12RendererDescriptor *descriptor, D3D12Device *device);
		D3DAPI ~D3D12Renderer();

		D3DAPI Window *CreateAWindow(const Vector2 &size, Screen *screen) final;
		D3DAPI Window *GetMainWindow() final;

		D3DAPI void RenderIntoWindow(Window *window, Function &&function) final;
		D3DAPI void RenderIntoCamera(Camera *camera, Function &&function) final;

		D3DAPI bool SupportsTextureFormat(const String *format) const final;
		D3DAPI bool SupportsDrawMode(DrawMode mode) const final;

		D3DAPI const String *GetTextureFormatName(const Texture::Format format) const final;
		D3DAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		D3DAPI size_t GetSizeForType(PrimitiveType type) const final;

		D3DAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions options) final;
		D3DAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options) final;

		D3DAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options) final;
		D3DAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options) final;

		D3DAPI ShaderProgram *GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup) final;

		D3DAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		D3DAPI Framebuffer *CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor) final;

		D3DAPI Drawable *CreateDrawable() final;
		D3DAPI void SubmitDrawable(Drawable *drawable) final;

		D3D12Device *GetD3D12Device() const { return static_cast<D3D12Device *>(GetDevice()); }
		D3D12RendererDescriptor *GetD3D12Descriptor() const { return static_cast<D3D12RendererDescriptor *>(GetDescriptor()); }

		ID3D12DescriptorHeap *GetRTVHeap() const { return _rtvHeap; }
		UINT GetRTVHeapSize() const { return _rtvDescriptorSize; }
		ID3D12DescriptorHeap *GetCBVHeap() const { return _cbvHeap; }

	protected:
		void RenderDrawable(D3D12Drawable *drawable);
		void FillUniformBuffer(D3D12UniformBuffer *buffer, D3D12Drawable *drawable);

		void CreateMipMapForeTexture(D3D12Texture *texture);
		void CreateMipMaps();

		Set *_mipMapTextures;
		Dictionary *_textureFormatLookup;

		D3D12Window *_mainWindow;

		SpinLock _lock;
		Dictionary *_defaultShaders;

		ID3D12RootSignature *_rootSignature;

		ID3D12DescriptorHeap *_rtvHeap;
		UINT _rtvDescriptorSize;
		ID3D12DescriptorHeap *_cbvHeap;
		UINT _cbvDescriptorSize;

		RNDeclareMetaAPI(D3D12Renderer, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12RENDERER_H__ */
