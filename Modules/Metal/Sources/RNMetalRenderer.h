//
//  RNMetalRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERER_H__
#define __RAYNE_METALRENDERER_H__

#include "RNMetal.h"
#include "RNMetalWindow.h"

namespace RN
{
	struct MetalRendererInternals;
	struct MetalDrawable;

	class MetalRendererDescriptor;
	class MetalDevice;
	class MetalWindow;
	class MetalTexture;
	class MetalUniformBuffer;
	struct MetalRenderPass;
	class GPUBuffer;
	class MetalUniformBufferReference;
	class MetalUniformBufferPool;

	class MetalRenderer : public Renderer
	{
	public:
		friend class MetalTexture;
		friend class MetalWindow;

		MTLAPI MetalRenderer(MetalRendererDescriptor *descriptor, MetalDevice *device);
		MTLAPI ~MetalRenderer();

		MTLAPI Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor()) final;
		MTLAPI Window *GetMainWindow() final;
		MTLAPI void SetMainWindow(Window *window) final;

		MTLAPI void Render(Function &&function) final;
		MTLAPI void SubmitCamera(Camera *camera, Function &&function) final;

		MTLAPI bool SupportsTextureFormat(const String *format) const final;
		MTLAPI bool SupportsDrawMode(DrawMode mode) const final;

		MTLAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		MTLAPI size_t GetSizeForType(PrimitiveType type) const final;

		MTLAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions, bool streameable) final;

		MTLAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file) final;
		MTLAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source) final;

		MTLAPI ShaderLibrary *GetDefaultShaderLibrary() final;

		MTLAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;
		MTLAPI Texture *CreateTextureWithDescriptorAndIOSurface(const Texture::Descriptor &descriptor, IOSurfaceRef ioSurface);

		MTLAPI Framebuffer *CreateFramebuffer(const Vector2 &size) final;

		MTLAPI Drawable *CreateDrawable() final;
		MTLAPI void DeleteDrawable(Drawable *drawable) final;
		MTLAPI void SubmitDrawable(Drawable *drawable) final;
		MTLAPI void SubmitLight(const Light *light) final;
		
		MTLAPI static MTLResourceOptions MetalResourceOptionsFromOptions(GPUResource::AccessOptions options);
		
		MTLAPI MetalUniformBufferReference *GetUniformBufferReference(size_t size, size_t index);
		MTLAPI void UpdateUniformBufferReference(MetalUniformBufferReference *reference, bool align);

	protected:
		void SubmitRenderPass(RenderPass *renderPass, MetalRenderPass &previousRenderPass, Function &&function);
		void RenderDrawable(MetalDrawable *drawable, uint32 instanceCount);
		void RenderAPIRenderPass(const MetalRenderPass &renderPass);
		void FillUniformBuffer(Shader::ArgumentBuffer *argument, MetalUniformBufferReference *uniformBufferReference, MetalDrawable *drawable, const Material::Properties &materialProperties);

		void CreateMipMapForTexture(MetalTexture *texture);
		void CreateMipMaps();

		Set *_mipMapTextures;

		PIMPL<MetalRendererInternals> _internals;
		Window *_mainWindow;
		
		MetalDrawable *_defaultPostProcessingDrawable;
		Material *_ppConvertMaterial;

		Lockable _lock;

		MetalUniformBufferPool *_uniformBufferPool;
		ShaderLibrary *_defaultShaderLibrary;
		
		uint8 _currentMultiviewLayer;
		RenderPass *_currentMultiviewFallbackRenderPass;

		RNDeclareMetaAPI(MetalRenderer, MTLAPI)
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */
