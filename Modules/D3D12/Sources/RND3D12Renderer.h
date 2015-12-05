//
//  RND3D12Renderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12RENDERER_H__
#define __RAYNE_D3D12RENDERER_H__

#include <Rayne.h>
#include "RND3D12Window.h"

namespace RN
{
	struct D3D12RendererInternals;
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

		RNAPI D3D12Renderer(const Dictionary *parameters);
		RNAPI ~D3D12Renderer();

		RNAPI Window *CreateAWindow(const Vector2 &size, Screen *screen) final;
		RNAPI Window *GetMainWindow() final;

		RNAPI void RenderIntoWindow(Window *window, Function &&function) final;
		RNAPI void RenderIntoCamera(Camera *camera, Function &&function) final;

		RNAPI bool SupportsTextureFormat(const String *format) const final;
		RNAPI bool SupportsDrawMode(DrawMode mode) const final;

		RNAPI const String *GetTextureFormatName(const Texture::Format format) const final;
		RNAPI size_t GetAlignmentForType(PrimitiveType type) const final;
		RNAPI size_t GetSizeForType(PrimitiveType type) const final;

		RNAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions options) final;
		RNAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options) final;

		RNAPI ShaderLibrary *CreateShaderLibraryWithFile(const String *file, const ShaderCompileOptions *options) final;
		RNAPI ShaderLibrary *CreateShaderLibraryWithSource(const String *source, const ShaderCompileOptions *options) final;

		RNAPI ShaderProgram *GetDefaultShader(const Mesh *mesh, const ShaderLookupRequest *lookup) final;

		RNAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		RNAPI Drawable *CreateDrawable() final;
		RNAPI void SubmitDrawable(Drawable *drawable) final;

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

		RNDeclareMeta(D3D12Renderer)
	};
}


#endif /* __RAYNE_D3D12RENDERER_H__ */
