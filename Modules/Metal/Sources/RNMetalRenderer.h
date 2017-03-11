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

		MTLAPI Window *CreateAWindow(const Vector2 &size, Screen *screen) final;
		MTLAPI Window *GetMainWindow() final;

		MTLAPI void RenderIntoWindow(Window *window, Function &&function) final;
		MTLAPI void RenderIntoCamera(Camera *camera, Function &&function) final;

		MTLAPI bool SupportsTextureFormat(const String *format) const final;
		MTLAPI bool SupportsDrawMode(DrawMode mode) const final;

		MTLAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		MTLAPI size_t GetSizeForType(PrimitiveType type) const final;

		MTLAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;
		MTLAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions usageOptions, GPUResource::AccessOptions accessOptions) final;

		MTLAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options) final;
		MTLAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options) final;

		MTLAPI ShaderProgram *GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup) final;

		MTLAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		MTLAPI Framebuffer *CreateFramebuffer(const Vector2 &size, const Framebuffer::Descriptor &descriptor) final;

		MTLAPI Drawable *CreateDrawable() final;
		MTLAPI void DeleteDrawable(Drawable *drawable);
		MTLAPI void SubmitDrawable(Drawable *drawable) final;

		const String *GetTextureFormatName(const Texture::Format format) const final;

	protected:
		void RenderDrawable(MetalDrawable *drawable);
		void FillUniformBuffer(MetalUniformBuffer *buffer, MetalDrawable *drawable);

		void CreateMipMapForTexture(MetalTexture *texture);
		void CreateMipMaps();

		Set *_mipMapTextures;
		Dictionary *_textureFormatLookup;

		PIMPL<MetalRendererInternals> _internals;
		MetalWindow *_mainWindow;

		Lockable _lock;

		Dictionary *_defaultShaders;

		RNDeclareMetaAPI(MetalRenderer, MTLAPI)
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */
