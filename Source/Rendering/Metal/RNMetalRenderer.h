//
//  RNMetalRenderer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERER_H__
#define __RAYNE_METALRENDERER_H__

#include "../../Base/RNBase.h"
#include "../RNRenderer.h"
#include "RNMetalWindow.h"

namespace RN
{
	struct MetalRendererInternals;
	struct MetalDrawable;
	class MetalWindow;
	class GPUBuffer;

	class MetalRenderer : public Renderer
	{
	public:
		friend class MetalWindow;

		RNAPI MetalRenderer();
		RNAPI ~MetalRenderer();

		RNAPI Window *CreateWindow(const Vector2 &size, Screen *screen) final;
		RNAPI Window *GetMainWindow() final;

		RNAPI void BeginWindow(Window *window) final;
		RNAPI void EndWindow() final;

		RNAPI void BeginCamera(Camera *camera) final;
		RNAPI void EndCamera() final;

		RNAPI bool SupportsTextureFormat(Texture::Format format) final;
		RNAPI bool SupportsDrawMode(DrawMode mode) final;

		RNAPI GPUBuffer *CreateBufferWithLength(size_t length, GPUResource::UsageOptions options) final;
		RNAPI GPUBuffer *CreateBufferWithBytes(const void *bytes, size_t length, GPUResource::UsageOptions options) final;

		RNAPI ShaderLibrary *GetShaderLibraryWithFile(const String *file) final;
		RNAPI ShaderLibrary *GetShaderLibraryWithSource(const String *source) final;

		RNAPI Texture *CreateTextureWithDescriptor(const Texture::Descriptor &descriptor) final;

		RNAPI Drawable *CreateDrawable() final;
		RNAPI void SubmitDrawable(Drawable *drawable) final;

	protected:
		void RenderDrawable(size_t index, MetalDrawable *drawable);

		PIMPL<MetalRendererInternals> _internals;
		MetalWindow *_mainWindow;

		std::vector<Camera *> _cameras;
		SpinLock _lock;

		RNDeclareMeta(MetalRenderer)
	};
}


#endif /* __RAYNE_METALRENDERER_H__ */
