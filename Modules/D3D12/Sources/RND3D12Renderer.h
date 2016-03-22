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

namespace RN
{
	class D3D12RendererInternals;
	struct D3D12Drawable;
	class D3D12Window;
	class D3D12Texture;
	class D3D12UniformBuffer;
	class GPUBuffer;

	class D3D12Renderer : public Renderer
	{
	public:
		friend class D3D12Texture;
		friend class D3D12Window;
		friend class D3D12StateCoordinator;
		friend class D3D12GPUBuffer;

		D3DAPI D3D12Renderer(const Dictionary *parameters);
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

	protected:
		void RenderDrawable(D3D12Drawable *drawable);
		void FillUniformBuffer(D3D12UniformBuffer *buffer, D3D12Drawable *drawable);

		void CreateMipMapForeTexture(D3D12Texture *texture);
		void CreateMipMaps();

		Set *_mipMapTextures;
		Dictionary *_textureFormatLookup;

		PIMPL<D3D12RendererInternals> _internals;
		D3D12Window *_mainWindow;

		SpinLock _lock;
		Dictionary *_defaultShaders;

		RNDeclareMeta(D3D12Renderer)
	};
}


#endif /* __RAYNE_D3D12RENDERER_H__ */
