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

namespace RN
{
	struct D3D12Drawable;
	struct D3D12RendererInternals;
	struct D3D12RootSignature;
	struct D3D12RenderPass;
	class D3D12Window;
	class D3D12Texture;
	class D3D12UniformBufferPool;
	class D3D12UniformBufferReference;
	class D3D12DescriptorHeap;
	class D3D12CommandList;
	class D3D12CommandListWithCallback;

	class D3D12Renderer : public Renderer
	{
	public:
		friend class D3D12Texture;
		friend class D3D12Window;
		friend class D3D12StateCoordinator;
		friend class D3D12GPUBuffer;

		D3DAPI D3D12Renderer(D3D12RendererDescriptor *descriptor, D3D12Device *device);
		D3DAPI ~D3D12Renderer();

		D3DAPI Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor()) final;
		D3DAPI void SetMainWindow(Window *window) final;
		D3DAPI Window *GetMainWindow() final;

		D3DAPI void Render(Function &&function) final;
		D3DAPI void SubmitCamera(Camera *camera, Function &&function) final;

		D3DAPI bool SupportsTextureFormat(const String *format) const final;
		D3DAPI bool SupportsDrawMode(DrawMode mode) const final;

		D3DAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		D3DAPI size_t GetSizeForType(PrimitiveType type) const final;

		D3DAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool isStreamable) final;
		D3DAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool isStreamable) final;

		D3DAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file) final;
		D3DAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source) final;

		D3DAPI ShaderLibrary *GetDefaultShaderLibrary();

		D3DAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		D3DAPI Framebuffer *CreateFramebuffer(const Vector2 &size) final;

		D3DAPI Drawable *CreateDrawable() final;
		D3DAPI void DeleteDrawable(Drawable *drawable) final;
		D3DAPI void SubmitDrawable(Drawable *drawable) final;
		D3DAPI void SubmitLight(const Light *light) final;

		ID3D12CommandQueue *GetCommandQueue() const { return _commandQueue; }
		D3D12DescriptorHeap *GetDescriptorHeap(size_t size, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		void SubmitDescriptorHeap(D3D12DescriptorHeap *heap);
		D3D12CommandList *GetCommandList();
		void SubmitCommandList(D3D12CommandList *commandBuffer);

		void AddFrameResouce(IUnknown *resource);

		D3D12Device *GetD3D12Device() const { return static_cast<D3D12Device *>(GetDevice()); }
		D3D12RendererDescriptor *GetD3D12Descriptor() const { return static_cast<D3D12RendererDescriptor *>(GetDescriptor()); }

		D3DAPI D3D12UniformBufferReference *GetUniformBufferReference(size_t size, size_t index);
		D3DAPI void UpdateUniformBufferReference(D3D12UniformBufferReference *reference, bool align);

	protected:
		void SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass);

		void RenderDrawable(ID3D12GraphicsCommandList *commandList, D3D12Drawable *drawable, uint32 instanceCount);
		void FillUniformBuffer(Shader::ArgumentBuffer *argument, D3D12UniformBufferReference *uniformBufferReference, D3D12Drawable *drawable);

		void RenderAPIRenderPass(D3D12CommandList *commandList, const D3D12RenderPass &renderPass);

		void CreateMipMapsForTexture(D3D12Texture *texture);
		void CreateMipMaps();

		void PopulateDescriptorHeap();
		void SetupRendertargets(D3D12CommandList *commandList, const D3D12RenderPass &renderpass);

		Array *_mipMapTextures;

		Window *_mainWindow;
		ShaderLibrary *_defaultShaderLibrary;

		D3D12Drawable *_defaultPostProcessingDrawable;
		Material *_ppConvertMaterial;

		ID3D12CommandQueue *_commandQueue;

		D3D12CommandList *_currentCommandList;
		Array *_submittedCommandLists;
		Array *_executedCommandLists;
		Array *_commandListPool;

		D3D12UniformBufferPool *_uniformBufferPool;

		PIMPL<D3D12RendererInternals> _internals;

		Lockable _lock;

		const D3D12RootSignature *_currentRootSignature;

		D3D12DescriptorHeap *_currentSrvCbvHeap;
		size_t _currentSrvCbvIndex;
		Array *_boundDescriptorHeaps;
		Array *_descriptorHeapPool;

		UINT _rtvDescriptorSize;

		ID3D12Fence *_fence;
		UINT _scheduledFenceValue;
		UINT _completedFenceValue;

		size_t _currentDrawableIndex;

		uint8 _currentMultiviewLayer;
		RenderPass *_currentMultiviewFallbackRenderPass;

		RNDeclareMetaAPI(D3D12Renderer, D3DAPI)
	};
}


#endif /* __RAYNE_D3D12RENDERER_H__ */
