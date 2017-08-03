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
	class GPUBuffer;

	class MetalRenderer : public Renderer
	{
	public:
		friend class MetalTexture;
		friend class MetalWindow;

		MTLAPI MetalRenderer(MetalRendererDescriptor *descriptor, MetalDevice *device);
		MTLAPI ~MetalRenderer();

		MTLAPI Window *CreateAWindow(const Vector2 &size, Screen *screen, const Window::SwapChainDescriptor &descriptor = Window::SwapChainDescriptor()) final;
		MTLAPI Window *GetMainWindow() final;

		MTLAPI void Render(Function &&function) final;
		MTLAPI void SubmitCamera(Camera *camera, Function &&function) final;
		MTLAPI void SubmitRenderPass(RenderPass *renderPass, RenderPass *previousRenderPass) final {}; //TODO: Implement

		MTLAPI bool SupportsTextureFormat(const String *format) const final;
		MTLAPI bool SupportsDrawMode(DrawMode mode) const final;

		MTLAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		MTLAPI size_t GetSizeForType(PrimitiveType type) const final;

		MTLAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;
		MTLAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;

		MTLAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file) final;
		MTLAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source) final;

		MTLAPI Shader *GetDefaultShader(Shader::Type type, Shader::Options *options, Shader::UsageHint hint) final;

		MTLAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		MTLAPI Framebuffer *CreateFramebuffer(const Vector2 &size) final;

		MTLAPI Drawable *CreateDrawable() final;
		MTLAPI void DeleteDrawable(Drawable *drawable) final;
		MTLAPI void SubmitDrawable(Drawable *drawable) final;
		MTLAPI void SubmitLight(const Light *light) final;

	protected:
		void RenderDrawable(MetalDrawable *drawable);
		void FillUniformBuffer(MetalUniformBuffer *buffer, MetalDrawable *drawable, Shader *shader);

		void CreateMipMapForTexture(MetalTexture *texture);
		void CreateMipMaps();

		Set *_mipMapTextures;

		PIMPL<MetalRendererInternals> _internals;
		MetalWindow *_mainWindow;

		Lockable _lock;

		ShaderLibrary *_defaultShaderLibrary;

		RNDeclareMetaAPI(MetalRenderer, MTLAPI)
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */
